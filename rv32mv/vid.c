// Copyright 2021 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51
//
// Author: Matheus Cavalcante <matheusd@iis.ee.ethz.ch>
//         Basile Bougenot <bbougenot@student.ethz.ch>

#include "vector_macros.h"

void TEST_CASE1() {
  VSET(16, e8, m1);
  asm volatile("vid.v v1");
  VCMP_U8(1, v1, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07);
}

void TEST_CASE2() {
  VSET(16, e8, m1);
  VLOAD_8(v0, 0x55, 0x00);
  VLOAD_8(v1, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef);
  asm volatile("vid.v v1, v0.t");
  VCMP_U8(2, v1, 0x00, 0xef, 0x02, 0xef, 0x04, 0xef, 0x06, 0xef);
}

int main(void) {
  INIT_CHECK();
  enable_vec();
  TEST_CASE1();
  TEST_CASE2();
  EXIT_CHECK();
}
