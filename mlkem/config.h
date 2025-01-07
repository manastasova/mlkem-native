/*
 * Copyright (c) 2024 The mlkem-native project authors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MLKEM_NATIVE_CONFIG_H
#define MLKEM_NATIVE_CONFIG_H

/******************************************************************************
 * Name:        MLKEM_K
 *
 * Description: Determines the security level for ML-KEM
 *              - MLKEM_K=2 corresponds to ML-KEM-512
 *              - MLKEM_K=3 corresponds to ML-KEM-768
 *              - MLKEM_K=4 corresponds to ML-KEM-1024
 *
 *              This can also be set using CFLAGS.
 *
 *****************************************************************************/
#ifndef MLKEM_K
#define MLKEM_K 3 /* Change this for different security strengths */
#endif

/******************************************************************************
 * Name:        MLKEM_NATIVE_CONFIG_FILE
 *
 * Description: If defined, this is a header that will be included instead
 *              of this default configuration file mlkem/config.h.
 *
 *              When you need to build mlkem-native in multiple configurations,
 *              using varying MLKEM_NATIVE_CONFIG_FILE can be more convenient
 *              then configuring everything through CFLAGS.
 *
 *              To use, MLKEM_NATIVE_CONFIG_FILE _must_ be defined prior
 *              to the inclusion of any mlkem-native headers. For example,
 *              it can be set by passing `-DMLKEM_NATIVE_CONFIG_FILE="..."`
 *              on the command line.
 *
 *****************************************************************************/
/* #define MLKEM_NATIVE_CONFIG_FILE "config.h" */

/******************************************************************************
 * Name:        MLKEM_NAMESPACE
 *
 * Description: The macros to use to namespace global symbols
 *              from mlkem/.
 *****************************************************************************/
#define MLKEM_NAMESPACE(sym) MLKEM_DEFAULT_NAMESPACE(sym)

/******************************************************************************
 * Name:        FIPS202_NAMESPACE
 *
 * Description: The macros to use to namespace global symbols
 *              from mlkem/fips202/.
 *****************************************************************************/
#define FIPS202_NAMESPACE(sym) FIPS202_DEFAULT_NAMESPACE(sym)

/******************************************************************************
 * Name:        MLKEM_USE_NATIVE
 *
 * Description: Determines whether a native backend should
 *              be used, if available.
 *
 *              This can also be set using CFLAGS.
 *
 *****************************************************************************/
#if !defined(MLKEM_USE_NATIVE)
/* #define MLKEM_USE_NATIVE */
#endif

/******************************************************************************
 * Name:        MLKEM_NATIVE_ARITH_BACKEND
 *
 * Description: The arithmetic backend to use.
 *
 *              This must be the filename of an arithmetic backend.
 *              See the existing backends for examples.
 *
 *              This can be set using CFLAGS.
 *
 *****************************************************************************/
#if defined(MLKEM_USE_NATIVE) && !defined(MLKEM_NATIVE_ARITH_BACKEND)
#define MLKEM_NATIVE_ARITH_BACKEND "native/default.h"
#endif /* MLKEM_NATIVE_ARITH_BACKEND */

/******************************************************************************
 * Name:        MLKEM_NATIVE_FIPS202_BACKEND
 *
 * Description: The FIPS-202 backend to use.
 *
 *              This must be the filename of an FIPS-202 backend.
 *
 *              This can be set using CFLAGS.
 *
 *****************************************************************************/
#if defined(MLKEM_USE_NATIVE) && !defined(MLKEM_NATIVE_FIPS202_BACKEND)
#define MLKEM_NATIVE_FIPS202_BACKEND "fips202/native/default.h"
#endif /* MLKEM_NATIVE_FIPS202_BACKEND */

/*************************  Config internals  ********************************/

/* Default namespace
 *
 * Don't change this. If you need a different namespace, re-define
 * MLKEM_NAMESPACE above instead, and remove the following.
 */

/*
 * The default FIPS202 namespace is
 *
 *   PQCP_MLKEM_NATIVE_FIPS202_<BACKEND>_
 *
 * e.g., PQCP_MLKEM_NATIVE_FIPS202_C_
 */

#define FIPS202_DEFAULT_NAMESPACE___(x1, x2) x1##_##x2
#define FIPS202_DEFAULT_NAMESPACE__(x1, x2) FIPS202_DEFAULT_NAMESPACE___(x1, x2)

#define FIPS202_DEFAULT_NAMESPACE(s) \
  FIPS202_DEFAULT_NAMESPACE__(PQCP_MLKEM_NATIVE_FIPS202, s)

/*
 * The default MLKEM namespace is
 *
 *   PQCP_MLKEM_NATIVE_MLKEM<LEVEL>_<BACKEND>_
 *
 * e.g., PQCP_MLKEM_NATIVE_MLKEM512_AARCH64_OPT_
 */

#define MLKEM_DEFAULT_NAMESPACE___(x1, x2, x3) x1##_##x2##_##x3
#define MLKEM_DEFAULT_NAMESPACE__(x1, x2, x3) \
  MLKEM_DEFAULT_NAMESPACE___(x1, x2, x3)

#if MLKEM_K == 2
#define MLKEM_DEFAULT_NAMESPACE(s) \
  MLKEM_DEFAULT_NAMESPACE__(PQCP_MLKEM_NATIVE, MLKEM512, s)
#elif MLKEM_K == 3
#define MLKEM_DEFAULT_NAMESPACE(s) \
  MLKEM_DEFAULT_NAMESPACE__(PQCP_MLKEM_NATIVE, MLKEM768, s)
#elif MLKEM_K == 4
#define MLKEM_DEFAULT_NAMESPACE(s) \
  MLKEM_DEFAULT_NAMESPACE__(PQCP_MLKEM_NATIVE, MLKEM1024, s)
#endif

#endif /* MLkEM_NATIVE_CONFIG_H */
