/*
 * Copyright (c) 2024 The mlkem-native project authors
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CBD_H
#define CBD_H

#include <stdint.h>
#include "common.h"
#include "poly.h"

#define poly_cbd2 MLKEM_NAMESPACE(poly_cbd2)
/*************************************************
 * Name:        poly_cbd2
 *
 * Description: Given an array of uniformly random bytes, compute
 *              polynomial with coefficients distributed according to
 *              a centered binomial distribution with parameter eta=2
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const uint8_t *buf: pointer to input byte array
 **************************************************/
MLKEM_NATIVE_INTERNAL_API
void poly_cbd2(poly *r, const uint8_t buf[2 * MLKEM_N / 4]);

#if defined(MLKEM_NATIVE_MULTILEVEL_BUILD_WITH_SHARED) || MLKEM_ETA1 == 3
#define poly_cbd3 MLKEM_NAMESPACE(poly_cbd3)
/*************************************************
 * Name:        poly_cbd3
 *
 * Description: Given an array of uniformly random bytes, compute
 *              polynomial with coefficients distributed according to
 *              a centered binomial distribution with parameter eta=3.
 *              This function is only needed for ML-KEM-512
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const uint8_t *buf: pointer to input byte array
 **************************************************/
MLKEM_NATIVE_INTERNAL_API
void poly_cbd3(poly *r, const uint8_t buf[3 * MLKEM_N / 4]);
#endif /* MLKEM_NATIVE_MULTILEVEL_BUILD || MLKEM_ETA1 == 3 */

#endif /* CBD_H */
