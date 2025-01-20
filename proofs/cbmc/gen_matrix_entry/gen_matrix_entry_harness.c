// Copyright (c) 2024 The mlkem-native project authors
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT-0

#include <stdint.h>
#include "poly.h"

// declare here since it's static in non-CBMC builds
#define poly_rej_uniform MLKEM_NAMESPACE(poly_rej_uniform)
void poly_rej_uniform(poly *entry, uint8_t seed[MLKEM_SYMBYTES + 16]);

void harness(void)
{
  poly *out;
  uint8_t *seed;
  poly_rej_uniform(out, seed);
}
