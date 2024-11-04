// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT-0 AND Apache-2.0

/*
 * Insert copyright notice
 */

/**
 * @file cmov_int16_harness.c
 * @brief Implements the proof harness for cmov_int16 function.
 */
#include "verify.h"

/*
 * Insert project header files that
 *   - include the declaration of the function
 *   - include the types needed to declare function arguments
 */

/**
 * @brief Starting point for formal analysis
 *
 */
void harness(void) {
  uint16_t b;
  int16_t *r, v;

  cmov_int16(r, v, b);
}
