#!/usr/bin/env bash
# Copyright (c) 2024-2025 The mlkem-native project authors
# SPDX-License-Identifier: Apache-2.0
#
# This tiny script just lists the names of source files for which
# we have a spec and proof in HOL-Light.

ROOT=$(git rev-parse --show-toplevel)
cd $ROOT
ls -1 proofs/hol_light/arm/mlkem/*.S | cut -d '/' -f 5 | sed 's/\.S//'
