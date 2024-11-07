// SPDX-License-Identifier: Apache-2.0
#include "poly.h"
#include <stdint.h>
#include <string.h>
#include "cbd.h"
#include "cbmc.h"
#include "fips202x4.h"
#include "ntt.h"
#include "params.h"
#include "reduce.h"
#include "symmetric.h"
#include "verify.h"

#include "arith_native.h"
#include "debug/debug.h"

/*************************************************
 * Name:        poly_compress
 *
 * Description: Compression and subsequent serialization of a polynomial
 *
 * Arguments:   - uint8_t *r: pointer to output byte array
 *                            (of length MLKEM_POLYCOMPRESSEDBYTES)
 *              - const poly *a: pointer to input polynomial
 *                  Coefficients must be unsigned canonical,
 *                  i.e. in [0,1,..,MLKEM_Q-1].
 **************************************************/
void poly_compress(uint8_t r[MLKEM_POLYCOMPRESSEDBYTES], const poly *a) {
  POLY_UBOUND(a, MLKEM_Q);
  uint8_t t[8] = {0};

  const int num_blocks = MLKEM_N / 8;

#if (MLKEM_POLYCOMPRESSEDBYTES == 128)
  for (int i = 0; i < num_blocks; i++)
      // clang-format off
        ASSIGNS(i, OBJECT_WHOLE(t), OBJECT_WHOLE(r))
        INVARIANT(i >= 0 && i <= num_blocks)
        DECREASES(num_blocks - i)
    // clang-format on
    {
      for (int j = 0; j < 8; j++)
          // clang-format off
            ASSIGNS(j, OBJECT_WHOLE(t))
            INVARIANT(i >= 0 && i <= num_blocks)
            INVARIANT(j >= 0 && j <= 8)
            INVARIANT(ARRAY_IN_BOUNDS(int, k2, 0, (j-1), t, 0, 15))
            DECREASES(8 - j)
        // clang-format on
        {
          // map to positive standard representatives
          // REF-CHANGE: Precondition change, we assume unsigned canonical data
          t[j] = scalar_compress_q_16(a->coeffs[8 * i + j]);
        }

      // REF-CHANGE: Use array indexing into
      // r rather than pointer-arithmetic to simplify verification
      r[i * 4] = t[0] | (t[1] << 4);
      r[i * 4 + 1] = t[2] | (t[3] << 4);
      r[i * 4 + 2] = t[4] | (t[5] << 4);
      r[i * 4 + 3] = t[6] | (t[7] << 4);
    }
#elif (MLKEM_POLYCOMPRESSEDBYTES == 160)
  for (int i = 0; i < num_blocks; i++)
      // clang-format off
        ASSIGNS(i, OBJECT_WHOLE(t), OBJECT_WHOLE(r))
        INVARIANT(i >= 0 && i <= num_blocks)
        DECREASES(num_blocks - i)
    // clang-format on
    {
      for (int j = 0; j < 8; j++)
          // clang-format off
            ASSIGNS(j, OBJECT_WHOLE(t))
            INVARIANT(i >= 0)
            INVARIANT(i <= num_blocks)
            INVARIANT(j >= 0 && j <= 8)
            INVARIANT(ARRAY_IN_BOUNDS(int, k2, 0, (j-1), t, 0, 31))
            DECREASES(8 - j)
        // clang-format on
        {
          // map to positive standard representatives
          // REF-CHANGE: Precondition change, we assume unsigned canonical data
          t[j] = scalar_compress_q_32(a->coeffs[8 * i + j]);
        }

      // REF-CHANGE: Explicitly truncate to avoid warning about
      // implicit truncation in CBMC, and use array indexing into
      // r rather than pointer-arithmetic to simplify verification
      r[i * 5] = 0xFF & ((t[0] >> 0) | (t[1] << 5));
      r[i * 5 + 1] = 0xFF & ((t[1] >> 3) | (t[2] << 2) | (t[3] << 7));
      r[i * 5 + 2] = 0xFF & ((t[3] >> 1) | (t[4] << 4));
      r[i * 5 + 3] = 0xFF & ((t[4] >> 4) | (t[5] << 1) | (t[6] << 6));
      r[i * 5 + 4] = 0xFF & ((t[6] >> 2) | (t[7] << 3));
    }
#else
#error "MLKEM_POLYCOMPRESSEDBYTES needs to be in {128, 160}"
#endif
}

/*************************************************
 * Name:        poly_decompress
 *
 * Description: De-serialization and subsequent decompression of a polynomial;
 *              approximate inverse of poly_compress
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const uint8_t *a: pointer to input byte array
 *                                  (of length MLKEM_POLYCOMPRESSEDBYTES bytes)
 *
 * Upon return, the coefficients of the output polynomial are unsigned-canonical
 * (non-negative and smaller than MLKEM_Q).
 *
 **************************************************/
void poly_decompress(poly *r, const uint8_t a[MLKEM_POLYCOMPRESSEDBYTES]) {
#if (MLKEM_POLYCOMPRESSEDBYTES == 128)
  for (int i = 0; i < MLKEM_N / 2; i++)
      // clang-format off
        ASSIGNS(i, OBJECT_WHOLE(r))
        INVARIANT(i >= 0)
        INVARIANT(i <= MLKEM_N / 2)
        INVARIANT(ARRAY_IN_BOUNDS(int, k, 0, (2 * i - 1), r->coeffs, 0, (MLKEM_Q - 1)))
        DECREASES(MLKEM_N / 2 - i)
    // clang-format on
    {
      // REF-CHANGE: Hoist scalar decompression into separate function
      r->coeffs[2 * i + 0] = scalar_decompress_q_16((a[i] >> 0) & 0xF);
      r->coeffs[2 * i + 1] = scalar_decompress_q_16((a[i] >> 4) & 0xF);
    }



#elif (MLKEM_POLYCOMPRESSEDBYTES == 160)
  const int num_blocks = MLKEM_N / 8;
  for (int i = 0; i < num_blocks; i++)
      // clang-format off
        ASSIGNS(i, OBJECT_WHOLE(r))
        INVARIANT(i >= 0)
        INVARIANT(i <= num_blocks)
        INVARIANT(num_blocks == 32)
        INVARIANT(ARRAY_IN_BOUNDS(int, k, 0, (8 * i - 1), r->coeffs, 0, (MLKEM_Q - 1)))
        DECREASES(num_blocks - i)
    // clang-format on
    {
      uint8_t t[8];
      const int offset = i * 5;
      // REF-CHANGE: Explicitly truncate to avoid warning about
      // implicit truncation in CBMC and unwind loop for ease
      // of proof.

      // Decompress 5 8-bit bytes (so 40 bits) into
      // 8 5-bit values stored in t[]
      t[0] = 0x1F & (a[offset + 0] >> 0);
      t[1] = 0x1F & ((a[offset + 0] >> 5) | (a[offset + 1] << 3));
      t[2] = 0x1F & (a[offset + 1] >> 2);
      t[3] = 0x1F & ((a[offset + 1] >> 7) | (a[offset + 2] << 1));
      t[4] = 0x1F & ((a[offset + 2] >> 4) | (a[offset + 3] << 4));
      t[5] = 0x1F & (a[offset + 3] >> 1);
      t[6] = 0x1F & ((a[offset + 3] >> 6) | (a[offset + 4] << 2));
      t[7] = 0x1F & (a[offset + 4] >> 3);

      // and copy to the correct slice in r[]
      for (int j = 0; j < 8; j++)
          // clang-format off
            ASSIGNS(j, OBJECT_WHOLE(r))
            INVARIANT(j >= 0)
            INVARIANT(j <= 8)
            INVARIANT(i >= 0)
            INVARIANT(i <= num_blocks)
            INVARIANT(ARRAY_IN_BOUNDS(int, k, 0, (8 * i + j - 1), r->coeffs, 0, (MLKEM_Q - 1)))
            DECREASES(8 - j)
        // clang-format on
        {
          // REF-CHANGE: Hoist scalar decompression into separate function
          r->coeffs[8 * i + j] = scalar_decompress_q_32(t[j]);
        }
    }
#else
#error "MLKEM_POLYCOMPRESSEDBYTES needs to be in {128, 160}"
#endif

  POLY_UBOUND(r, MLKEM_Q);
}

/*************************************************
 * Name:        poly_tobytes
 *
 * Description: Serialization of a polynomial.
 *              Signed coefficients are converted to
 *              unsigned form before serialization.
 *
 * Arguments:   INPUT:
 *              - a: const pointer to input polynomial,
 *                with each coefficient in the range [0,1,..,Q-1]
 *              OUTPUT
 *              - r: pointer to output byte array
 *                   (of MLKEM_POLYBYTES bytes)
 **************************************************/
#if !defined(MLKEM_USE_NATIVE_POLY_TOBYTES)
void poly_tobytes(uint8_t r[MLKEM_POLYBYTES], const poly *a) {
  POLY_UBOUND(a, MLKEM_Q);

  for (unsigned int i = 0; i < MLKEM_N / 2; i++)
      // clang-format off
  ASSIGNS(i, OBJECT_WHOLE(r))
  INVARIANT(i >= 0 && i <= MLKEM_N / 2)
  DECREASES(MLKEM_N / 2 - i)
    // clang-format on
    {
      const uint16_t t0 = a->coeffs[2 * i];
      const uint16_t t1 = a->coeffs[2 * i + 1];
      // REF-CHANGE: Precondition change, we assume unsigned canonical data

      // t0 and t1 are both < MLKEM_Q, so contain at most 12 bits each of
      // significant data, so these can be packed into 24 bits or exactly
      // 3 bytes, as follows.

      // Least significant bits 0 - 7 of t0.
      r[3 * i + 0] = t0 & 0xFF;

      // Most significant bits 8 - 11 of t0 become the least significant
      // nibble of the second byte. The least significant 4 bits
      // of t1 become the upper nibble of the second byte.
      r[3 * i + 1] = (t0 >> 8) | ((t1 << 4) & 0xF0);

      // Bits 4 - 11 of t1 become the third byte.
      r[3 * i + 2] = t1 >> 4;
    }
}
#else  /* MLKEM_USE_NATIVE_POLY_TOBYTES */
void poly_tobytes(uint8_t r[MLKEM_POLYBYTES], const poly *a) {
  POLY_UBOUND(a, MLKEM_Q);
  poly_tobytes_native(r, a);
}
#endif /* MLKEM_USE_NATIVE_POLY_TOBYTES */

#if !defined(MLKEM_USE_NATIVE_POLY_FROMBYTES)
/*************************************************
 * Name:        poly_frombytes
 *
 * Description: De-serialization of a polynomial.
 *
 * Arguments:   INPUT
 *              - a: pointer to input byte array
 *                   (of MLKEM_POLYBYTES bytes)
 *              OUTPUT
 *              - r: pointer to output polynomial, with
 *                   each coefficient unsigned and in the range
 *                   0 .. 4095
 **************************************************/
void poly_frombytes(poly *r, const uint8_t a[MLKEM_POLYBYTES]) {
  int i;
  for (i = 0; i < MLKEM_N / 2; i++)
      // clang-format off
    ASSIGNS(i, OBJECT_WHOLE(r))
    INVARIANT(i >= 0 && i <= MLKEM_N / 2)
    INVARIANT(ARRAY_IN_BOUNDS(int, k, 0, (2 * i - 1), r->coeffs, 0, 4095))
    DECREASES(MLKEM_N / 2 - i)
    // clang-format on
    {
      // REF-CHANGE: Introduce some locals for better readability
      const uint8_t t0 = a[3 * i + 0];
      const uint8_t t1 = a[3 * i + 1];
      const uint8_t t2 = a[3 * i + 2];
      r->coeffs[2 * i + 0] = t0 | ((t1 << 8) & 0xFFF);
      r->coeffs[2 * i + 1] = (t1 >> 4) | (t2 << 4);
    }

  // Note that the coefficients are not canonical
  POLY_UBOUND(r, 4096);
}
#else  /* MLKEM_USE_NATIVE_POLY_FROMBYTES */
void poly_frombytes(poly *r, const uint8_t a[MLKEM_POLYBYTES]) {
  poly_frombytes_native(r, a);
}
#endif /* MLKEM_USE_NATIVE_POLY_FROMBYTES */

void poly_frommsg(poly *r, const uint8_t msg[MLKEM_INDCPA_MSGBYTES]) {
#if (MLKEM_INDCPA_MSGBYTES != MLKEM_N / 8)
#error "MLKEM_INDCPA_MSGBYTES must be equal to MLKEM_N/8 bytes!"
#endif

  for (int i = 0; i < MLKEM_N / 8; i++)  // clang-format off
    ASSIGNS(i, OBJECT_WHOLE(r))
    INVARIANT(i >= 0 && i <= MLKEM_N / 8)
    INVARIANT(ARRAY_IN_BOUNDS(int, k, 0, (8 * i - 1), r->coeffs, 0, (MLKEM_Q - 1)))
    {      // clang-format on
      for (int j = 0; j < 8; j++)  // clang-format off
        ASSIGNS(j, OBJECT_WHOLE(r))
        INVARIANT(i >= 0 && i <  MLKEM_N / 8 && j >= 0 && j <= 8)
        INVARIANT(ARRAY_IN_BOUNDS(int, k, 0, (8 * i + j - 1), r->coeffs, 0, (MLKEM_Q - 1)))
        {  // clang-format on
          r->coeffs[8 * i + j] = 0;
          cmov_int16(&r->coeffs[8 * i + j], HALF_Q, (msg[i] >> j) & 1);
        }
    }
  POLY_BOUND_MSG(r, MLKEM_Q, "poly_frommsg output");
}

void poly_tomsg(uint8_t msg[MLKEM_INDCPA_MSGBYTES], const poly *a) {
  POLY_UBOUND(a, MLKEM_Q);

  for (int i = 0; i < MLKEM_N / 8; i++)
      // clang-format off
    ASSIGNS(i, OBJECT_WHOLE(msg))
    INVARIANT(i >= 0 && i <= MLKEM_N / 8)
    // clang-format on
    {
      msg[i] = 0;
      for (int j = 0; j < 8; j++)
          // clang-format off
        ASSIGNS(j, OBJECT_WHOLE(msg))
        INVARIANT(i >= 0 && i <= MLKEM_N / 8 && j >= 0 && j <= 8)
        // clang-format on
        {
          uint32_t coeff = a->coeffs[8 * i + j];

          // We now apply the "Compress1" function from FIPS-203 (eq 4.7)
          // A simple expansion would be:
          //   t += ((int16_t)t >> 15) & MLKEM_Q;
          //   t  = (((t << 1) + MLKEM_Q/2)/MLKEM_Q) & 1;
          // but we require constant-time evaluation, avoiding the division
          // operator. We therefore use a Montgomery division, using magic
          // multiplier floor(2**28 / MLKEM_Q) == 80635

          uint32_t t = coeff << 1;
          t += HALF_Q;
          t *= 80635;
          t >>= 28;
          t &= 1;

          // We can prove equivalence of these two forms, thus:
          ASSERT(t == (((((coeff + (((int16_t)coeff >> 15) & MLKEM_Q)) << 1) +
                         MLKEM_Q / 2) /
                        MLKEM_Q) &
                       1),
                 "axiomatic definition of Compress1");

          msg[i] |= t << j;
        }
    }
}

void poly_getnoise_eta1_4x(poly *r0, poly *r1, poly *r2, poly *r3,
                           const uint8_t seed[MLKEM_SYMBYTES], uint8_t nonce0,
                           uint8_t nonce1, uint8_t nonce2, uint8_t nonce3) {
  uint8_t buf[KECCAK_WAY][MLKEM_ETA1 * MLKEM_N / 4] ALIGN;
  uint8_t extkey[KECCAK_WAY][MLKEM_SYMBYTES + 1] ALIGN;
  memcpy(extkey[0], seed, MLKEM_SYMBYTES);
  memcpy(extkey[1], seed, MLKEM_SYMBYTES);
  memcpy(extkey[2], seed, MLKEM_SYMBYTES);
  memcpy(extkey[3], seed, MLKEM_SYMBYTES);
  extkey[0][MLKEM_SYMBYTES] = nonce0;
  extkey[1][MLKEM_SYMBYTES] = nonce1;
  extkey[2][MLKEM_SYMBYTES] = nonce2;
  extkey[3][MLKEM_SYMBYTES] = nonce3;
  shake256x4(buf[0], buf[1], buf[2], buf[3], MLKEM_ETA1 * MLKEM_N / 4,
             extkey[0], extkey[1], extkey[2], extkey[3], MLKEM_SYMBYTES + 1);
  poly_cbd_eta1(r0, buf[0]);
  poly_cbd_eta1(r1, buf[1]);
  poly_cbd_eta1(r2, buf[2]);
  poly_cbd_eta1(r3, buf[3]);

  POLY_BOUND_MSG(r0, MLKEM_ETA1 + 1, "poly_getnoise_eta1_4x output 0");
  POLY_BOUND_MSG(r1, MLKEM_ETA1 + 1, "poly_getnoise_eta1_4x output 1");
  POLY_BOUND_MSG(r2, MLKEM_ETA1 + 1, "poly_getnoise_eta1_4x output 2");
  POLY_BOUND_MSG(r3, MLKEM_ETA1 + 1, "poly_getnoise_eta1_4x output 3");
}

void poly_getnoise_eta2(poly *r, const uint8_t seed[MLKEM_SYMBYTES],
                        uint8_t nonce) {
  uint8_t buf[MLKEM_ETA2 * MLKEM_N / 4] ALIGN;
  prf(buf, sizeof(buf), seed, nonce);
  poly_cbd_eta2(r, buf);

  POLY_BOUND_MSG(r, MLKEM_ETA1 + 1, "poly_getnoise_eta2 output");
}

void poly_getnoise_eta2_4x(poly *r0, poly *r1, poly *r2, poly *r3,
                           const uint8_t seed[MLKEM_SYMBYTES], uint8_t nonce0,
                           uint8_t nonce1, uint8_t nonce2, uint8_t nonce3) {
  uint8_t buf[KECCAK_WAY][MLKEM_ETA2 * MLKEM_N / 4] ALIGN;
  uint8_t extkey[KECCAK_WAY][MLKEM_SYMBYTES + 1] ALIGN;
  memcpy(extkey[0], seed, MLKEM_SYMBYTES);
  memcpy(extkey[1], seed, MLKEM_SYMBYTES);
  memcpy(extkey[2], seed, MLKEM_SYMBYTES);
  memcpy(extkey[3], seed, MLKEM_SYMBYTES);
  extkey[0][MLKEM_SYMBYTES] = nonce0;
  extkey[1][MLKEM_SYMBYTES] = nonce1;
  extkey[2][MLKEM_SYMBYTES] = nonce2;
  extkey[3][MLKEM_SYMBYTES] = nonce3;
  shake256x4(buf[0], buf[1], buf[2], buf[3], MLKEM_ETA2 * MLKEM_N / 4,
             extkey[0], extkey[1], extkey[2], extkey[3], MLKEM_SYMBYTES + 1);
  poly_cbd_eta2(r0, buf[0]);
  poly_cbd_eta2(r1, buf[1]);
  poly_cbd_eta2(r2, buf[2]);
  poly_cbd_eta2(r3, buf[3]);

  POLY_BOUND_MSG(r0, MLKEM_ETA2 + 1, "poly_getnoise_eta2_4x output 0");
  POLY_BOUND_MSG(r1, MLKEM_ETA2 + 1, "poly_getnoise_eta2_4x output 1");
  POLY_BOUND_MSG(r2, MLKEM_ETA2 + 1, "poly_getnoise_eta2_4x output 2");
  POLY_BOUND_MSG(r3, MLKEM_ETA2 + 1, "poly_getnoise_eta2_4x output 3");
}

void poly_getnoise_eta1122_4x(poly *r0, poly *r1, poly *r2, poly *r3,
                              const uint8_t seed[MLKEM_SYMBYTES],
                              uint8_t nonce0, uint8_t nonce1, uint8_t nonce2,
                              uint8_t nonce3) {
  uint8_t buf1[KECCAK_WAY / 2][MLKEM_ETA1 * MLKEM_N / 4] ALIGN;
  uint8_t buf2[KECCAK_WAY / 2][MLKEM_ETA2 * MLKEM_N / 4] ALIGN;
  uint8_t extkey[KECCAK_WAY][MLKEM_SYMBYTES + 1] ALIGN;
  memcpy(extkey[0], seed, MLKEM_SYMBYTES);
  memcpy(extkey[1], seed, MLKEM_SYMBYTES);
  memcpy(extkey[2], seed, MLKEM_SYMBYTES);
  memcpy(extkey[3], seed, MLKEM_SYMBYTES);
  extkey[0][MLKEM_SYMBYTES] = nonce0;
  extkey[1][MLKEM_SYMBYTES] = nonce1;
  extkey[2][MLKEM_SYMBYTES] = nonce2;
  extkey[3][MLKEM_SYMBYTES] = nonce3;

#if MLKEM_ETA1 == MLKEM_ETA2
  shake256x4(buf1[0], buf1[1], buf2[0], buf2[1], MLKEM_ETA1 * MLKEM_N / 4,
             extkey[0], extkey[1], extkey[2], extkey[3], MLKEM_SYMBYTES + 1);
#else
  shake256(buf1[0], sizeof(buf1[0]), extkey[0], sizeof(extkey[0]));
  shake256(buf1[1], sizeof(buf1[1]), extkey[1], sizeof(extkey[1]));
  shake256(buf2[0], sizeof(buf2[0]), extkey[2], sizeof(extkey[2]));
  shake256(buf2[1], sizeof(buf2[1]), extkey[3], sizeof(extkey[3]));
#endif

  poly_cbd_eta1(r0, buf1[0]);
  poly_cbd_eta1(r1, buf1[1]);
  poly_cbd_eta2(r2, buf2[0]);
  poly_cbd_eta2(r3, buf2[1]);

  POLY_BOUND_MSG(r0, MLKEM_ETA1 + 1, "poly_getnoise_eta1122_4x output 0");
  POLY_BOUND_MSG(r1, MLKEM_ETA1 + 1, "poly_getnoise_eta1122_4x output 1");
  POLY_BOUND_MSG(r2, MLKEM_ETA2 + 1, "poly_getnoise_eta1122_4x output 2");
  POLY_BOUND_MSG(r3, MLKEM_ETA2 + 1, "poly_getnoise_eta1122_4x output 3");
}

void poly_basemul_montgomery_cached(poly *r, const poly *a, const poly *b,
                                    const poly_mulcache *b_cache) {
  int i;
  for (i = 0; i < MLKEM_N / 4; i++)
      // clang-format off
    ASSIGNS(i, OBJECT_WHOLE(r))
    INVARIANT(i >= 0 && i <= MLKEM_N / 4)
    INVARIANT(ARRAY_IN_BOUNDS(int, k, 0, (4 * i - 1), r->coeffs, -3 * HALF_Q + 1, 3 * HALF_Q - 1))
    DECREASES(MLKEM_N / 4 - i)
    // clang-format on
    {
      basemul_cached(&r->coeffs[4 * i], &a->coeffs[4 * i], &b->coeffs[4 * i],
                     b_cache->coeffs[2 * i]);
      basemul_cached(&r->coeffs[4 * i + 2], &a->coeffs[4 * i + 2],
                     &b->coeffs[4 * i + 2], b_cache->coeffs[2 * i + 1]);
    }
}

#if !defined(MLKEM_USE_NATIVE_POLY_TOMONT)
void poly_tomont(poly *r) {
  int i;
  const int16_t f = (1ULL << 32) % MLKEM_Q;  // 1353
  for (i = 0; i < MLKEM_N; i++)
      // clang-format off
    ASSIGNS(i, OBJECT_WHOLE(r))
    INVARIANT(i >= 0 && i <= MLKEM_N)
    INVARIANT(ARRAY_IN_BOUNDS(int, k, 0, (i - 1), r->coeffs, -(MLKEM_Q - 1), (MLKEM_Q - 1)))
    DECREASES(MLKEM_N - i)
    // clang-format on
    {
      r->coeffs[i] = fqmul(r->coeffs[i], f);
    }

  POLY_BOUND(r, MLKEM_Q);
}
#else  /* MLKEM_USE_NATIVE_POLY_TOMONT */
void poly_tomont(poly *r) {
  poly_tomont_native(r);
  POLY_BOUND(r, MLKEM_Q);
}
#endif /* MLKEM_USE_NATIVE_POLY_TOMONT */

#if !defined(MLKEM_USE_NATIVE_POLY_REDUCE)
void poly_reduce(poly *r) {
  int i;
  for (i = 0; i < MLKEM_N; i++)
      // clang-format off
    ASSIGNS(i, OBJECT_WHOLE(r))
    INVARIANT(i >= 0 && i <= MLKEM_N)
    INVARIANT(ARRAY_IN_BOUNDS(int, k, 0, (i - 1), r->coeffs, 0, MLKEM_Q - 1))
    DECREASES(MLKEM_N - i)
    // clang-format on
    {
      // Barrett reduction, giving signed canonical representative
      int16_t t = barrett_reduce(r->coeffs[i]);
      // Conditional addition to get unsigned canonical representative
      r->coeffs[i] = scalar_signed_to_unsigned_q_16(t);
    }

  POLY_UBOUND(r, MLKEM_Q);
}
#else  /* MLKEM_USE_NATIVE_POLY_REDUCE */
void poly_reduce(poly *r) {
  poly_reduce_native(r);
  POLY_UBOUND(r, MLKEM_Q);
}
#endif /* MLKEM_USE_NATIVE_POLY_REDUCE */

void poly_add(poly *r, const poly *b) {
  int i;
  for (i = 0; i < MLKEM_N; i++)
      // clang-format off
    ASSIGNS(i, OBJECT_WHOLE(r))
    INVARIANT(i >= 0 && i <= MLKEM_N)
    INVARIANT(FORALL(int, k0, i, MLKEM_N - 1, r->coeffs[k0] == LOOP_ENTRY(*r).coeffs[k0]))
    INVARIANT(FORALL(int, k1, 0, i - 1, r->coeffs[k1] == LOOP_ENTRY(*r).coeffs[k1] + b->coeffs[k1]))
    DECREASES(MLKEM_N - i)
    // clang-format on
    {
      r->coeffs[i] = r->coeffs[i] + b->coeffs[i];
    }
}


void poly_sub(poly *r, const poly *b) {
  int i;
  for (i = 0; i < MLKEM_N; i++)
      // clang-format off
    ASSIGNS(i, OBJECT_WHOLE(r))
    INVARIANT(i >= 0 && i <= MLKEM_N)
    INVARIANT(FORALL(int, k0, i, MLKEM_N - 1, r->coeffs[k0] == LOOP_ENTRY(*r).coeffs[k0]))
    INVARIANT(FORALL(int, k1, 0, i - 1, r->coeffs[k1] == LOOP_ENTRY(*r).coeffs[k1] - b->coeffs[k1]))
    DECREASES(MLKEM_N - i)
    // clang-format on
    {
      r->coeffs[i] = r->coeffs[i] - b->coeffs[i];
    }
}

/*************************************************
 * Name:        poly_mulcache_compute
 *
 * Description: Precompute values speeding up
 *              base multiplications of polynomials
 *              in NTT domain.
 *
 *              The coefficients in the mulcache must be
 *              bound by MLKEM_Q in absolute value.
 *
 * Arguments: - poly_mulcache *x: pointer to output cache.
 *            - const poly *a: pointer to input polynomial
 **************************************************/
#if !defined(MLKEM_USE_NATIVE_POLY_MULCACHE_COMPUTE)
void poly_mulcache_compute(poly_mulcache *x, const poly *a) {
  int i;
  for (i = 0; i < MLKEM_N / 4; i++)
      // clang-format off
    ASSIGNS(i, OBJECT_WHOLE(x))
    INVARIANT(i >= 0 && i <= MLKEM_N / 4)
    DECREASES(MLKEM_N / 4 - i)
    // clang-format on
    {
      x->coeffs[2 * i + 0] = fqmul(a->coeffs[4 * i + 1], zetas[64 + i]);
      x->coeffs[2 * i + 1] = fqmul(a->coeffs[4 * i + 3], -zetas[64 + i]);
    }
  POLY_BOUND(x, MLKEM_Q);
}
#else  /* MLKEM_USE_NATIVE_POLY_MULCACHE_COMPUTE */
void poly_mulcache_compute(poly_mulcache *x, const poly *a) {
  poly_mulcache_compute_native(x, a);
  POLY_BOUND(x, MLKEM_Q);
}
#endif /* MLKEM_USE_NATIVE_POLY_MULCACHE_COMPUTE */
