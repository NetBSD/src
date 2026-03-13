/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Architecture tests for intel i386 platform.  */

#include <stdlib.h>

char global_buf0[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
char global_buf1[] = {0, 0, 0, 0, 0, 0, 0, 0,
		      0, 0, 0, 0, 0, 0, 0, 0,
		      0, 0, 0, 0, 0, 0, 0, 0,
		      0, 0, 0, 0, 0, 0, 0, 0};
char *dyn_buf0;
char *dyn_buf1;

int
vmov_test ()
{
  char buf0[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
		 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f};
  char buf1[] = {0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0};

  /*start vmov_test.  */

  /* Operations on registers.  */
  asm volatile ("mov $0, %rcx");
  asm volatile ("mov $0xbeef, %rax");
  asm volatile ("vmovd %rax, %xmm0");
  asm volatile ("vmovd %xmm0, %rcx");
  asm volatile ("vmovq %xmm0, %xmm15");
  asm volatile ("vmovq %0, %%xmm15": : "m" (buf1));

  /* Operations based on local buffers.  */
  asm volatile ("vmovd %0, %%xmm0": : "m"(buf0));
  asm volatile ("vmovd %%xmm0, %0": "=m"(buf1));
  asm volatile ("vmovq %0, %%xmm0": : "m"(buf0));
  asm volatile ("vmovq %%xmm0, %0": "=m"(buf1));

  /* Operations based on global buffers.  */
  asm volatile ("vmovd %0, %%xmm0": : "m"(global_buf0));
  asm volatile ("vmovd %%xmm0, %0": "=m"(global_buf1));
  asm volatile ("vmovq %0, %%xmm0": : "m"(global_buf0));
  asm volatile ("vmovq %%xmm0, %0": "=m"(global_buf1));

  /* Operations based on dynamic buffers.  */
  asm volatile ("vmovd %0, %%xmm0": : "m"(*dyn_buf0));
  asm volatile ("vmovd %%xmm0, %0": "=m"(*dyn_buf1));
  asm volatile ("vmovq %0, %%xmm0": : "m"(*dyn_buf0));
  asm volatile ("vmovq %%xmm0, %0": "=m"(*dyn_buf1));

  /* Reset all relevant buffers. */
  asm volatile ("vmovq %%xmm15, %0": "=m" (buf1));
  asm volatile ("vmovq %%xmm15, %0": "=m" (global_buf1));
  asm volatile ("vmovq %%xmm15, %0": "=m" (*dyn_buf1));

  /* Quick test for a different xmm register.  */
  asm volatile ("vmovd %0, %%xmm15": "=m" (buf0));
  asm volatile ("vmovd %0, %%xmm15": "=m" (buf1));
  asm volatile ("vmovq %0, %%xmm15": "=m" (buf0));
  asm volatile ("vmovq %0, %%xmm15": "=m" (buf1));

  /* Test vmovdq style instructions.  */
  /* For local and global buffers, we can't guarantee they will be aligned.
     However, the aligned and unaligned versions seem to be encoded the same,
     so testing one is enough to validate both.  For safety, though, the
     dynamic buffers are forced to be 32-bit aligned so vmovdqa can be
     explicitly tested at least once.  */

  /* Operations based on local buffers.  */
  asm volatile ("vmovdqu %0, %%ymm0": : "m"(buf0));
  asm volatile ("vmovdqu %%ymm0, %0": "=m"(buf1));

  /* Operations based on global buffers.  */
  asm volatile ("vmovdqu %0, %%ymm0": : "m"(global_buf0));
  asm volatile ("vmovdqu %%ymm0, %0": "=m"(global_buf1));

  /* Operations based on dynamic buffers.  */
  asm volatile ("vmovdqa %0, %%ymm15": : "m"(*dyn_buf0));
  asm volatile ("vmovdqa %%ymm15, %0": "=m"(*dyn_buf1));
  asm volatile ("vmovdqu %0, %%ymm0": : "m"(*dyn_buf0));
  asm volatile ("vmovdqu %%ymm0, %0": "=m"(*dyn_buf1));

  /* Operations between 2 registers.  */
  asm volatile ("vmovdqu %ymm15, %ymm0");
  asm volatile ("vmovdqu %ymm2, %ymm15");
  asm volatile ("vmovdqa %ymm15, %ymm0");

  /* We have a return statement to deal with
     epilogue in different compilers.  */
  return 0; /* end vmov_test */
}

/* Test if we can properly record (and undo) vpunpck style instructions.
   Most tests will use xmm0 and xmm1 as sources. The registers xmm15 and xmm2
   are used as destination to ensure we're reading the VEX.R bit correctly.  */
int
vpunpck_test  ()
{
  /* Using GDB, load these values onto registers, for ease of testing.
     ymm0.v2_int128  = {0x1f1e1d1c1b1a19181716151413121110, 0x2f2e2d2c2b2a29282726252423222120}
     ymm1.v2_int128  = {0x4f4e4d4c4b4a49484746454443424140, 0x3f3e3d3c3b3a39383736353433323130}
     ymm15.v2_int128 = {0xdead, 0xbeef}
     so that's easy to confirm that the unpacking went as expected.  */

  /* start vpunpck_test.  */

  /* First try all low bit unpack instructions with xmm registers.  */
  /* 17 27 16 26 15 25 14 24 ...*/
  asm volatile ("vpunpcklbw  %xmm0, %xmm1, %xmm15");
  /* 17 16 27 26 15 14 25 24 ...*/
  asm volatile ("vpunpcklwd  %0, %%xmm1, %%xmm15"
      : : "m" (global_buf0));
  /* 17 16 15 14 27 26 25 24 ...*/
  asm volatile ("vpunpckldq  %0, %%xmm1,  %%xmm2"
      : : "m" (global_buf0));
  /* 17 16 15 14 13 12 11 10 ...*/
  asm volatile ("vpunpcklqdq %xmm0, %xmm1, %xmm2");

  /* Then try all high bit unpack instructions with xmm registers.  */
  /* 17 27 16 26 15 25 14 24 ...*/
  asm volatile ("vpunpckhbw  %xmm0, %xmm1, %xmm15");
  /* 17 16 27 26 15 14 25 24 ...*/
  asm volatile ("vpunpckhwd  %0, %%xmm1, %%xmm15"
      : : "m" (global_buf0));
  /* 17 16 15 14 27 26 25 24 ...*/
  asm volatile ("vpunpckhdq  %0, %%xmm1,  %%xmm2"
      : : "m" (global_buf0));
  /* 17 16 15 14 13 12 11 10 ...*/
  asm volatile ("vpunpckhqdq %xmm0, %xmm1, %xmm2");

  /* Lastly, lets test a few unpack instructions with ymm registers.  */
  /* 17 27 16 26 15 25 14 24 ...*/
  asm volatile ("vpunpcklbw  %ymm0, %ymm1, %ymm15");
  /* 17 16 27 26 15 14 25 24 ...*/
  asm volatile ("vpunpcklwd  %ymm0, %ymm1, %ymm15");
  /* 17 16 15 14 27 26 25 24 ...*/
  asm volatile ("vpunpckhdq  %ymm0, %ymm1,  %ymm15");
  /* 17 16 15 14 13 12 11 10 ...*/
  asm volatile ("vpunpckhqdq %ymm0, %ymm1, %ymm15");
  /* We have a return statement to deal with
     epilogue in different compilers.  */
  return 0; /* end vpunpck_test */
}

/* Test if we can record vpbroadcast instructions.  */
int
vpbroadcast_test ()
{
  /* Using GDB, load this value onto the register, for ease of testing.
     xmm0.uint128  = 0x0
     xmm1.uint128  = 0x1f1e1d1c1b1a19181716151413121110
     xmm15.uint128 = 0x0
     this way it's easy to confirm we're undoing things correctly.  */
  /* start vpbroadcast_test.  */

  asm volatile ("vpbroadcastb %xmm1, %xmm0");
  asm volatile ("vpbroadcastb %xmm1, %xmm15");

  asm volatile ("vpbroadcastw %xmm1, %ymm0");
  asm volatile ("vpbroadcastw %xmm1, %ymm15");

  asm volatile ("vpbroadcastd %xmm1, %xmm0");
  asm volatile ("vpbroadcastd %xmm1, %xmm15");

  asm volatile ("vpbroadcastq %xmm1, %ymm0");
  asm volatile ("vpbroadcastq %xmm1, %ymm15");

  /* We have a return statement to deal with
     epilogue in different compilers.  */
  return 0; /* end vpbroadcast_test */
}

int
vzeroupper_test ()
{
  /* start vzeroupper_test.  */
  /* Using GDB, load this value onto the register, for ease of testing.
     ymm0.v2_int128  = {0x0, 0x12345}
     ymm1.v2_int128  = {0x1f1e1d1c1b1a1918, 0x0}
     ymm2.v2_int128  = {0x0, 0xbeef}
     ymm15.v2_int128 = {0x0, 0xcafeface}
     this way it's easy to confirm we're undoing things correctly.  */

  asm volatile ("vzeroupper");

  /* We have a return statement to deal with
     epilogue in different compilers.  */
  return 0; /* end vzeroupper_test  */
}

int
vpor_xor_test ()
{
  /* start vpor_xor_test.  */
  /* Using GDB, load this value onto the register, for ease of testing.
     ymm0.v2_int128  = {0x0, 0x12345}
     ymm1.v2_int128  = {0x1f1e1d1c1b1a1918, 0x0}
     ymm2.v2_int128  = {0x0, 0xbeef}
     ymm15.v2_int128 = {0x0, 0xcafeface}
     this way it's easy to confirm we're undoing things correctly.  */

  asm volatile ("vpxor %ymm0, %ymm0, %ymm0");
  asm volatile ("vpxor %xmm0, %xmm1, %xmm0");
  asm volatile ("vpxor %ymm2, %ymm15, %ymm1");
  asm volatile ("vpxor %xmm2, %xmm15, %xmm2");
  asm volatile ("vpxor %ymm2, %ymm1, %ymm15");

  asm volatile ("vpor %ymm0, %ymm0, %ymm0");
  asm volatile ("vpor %xmm0, %xmm1, %xmm0");
  asm volatile ("vpor %ymm2, %ymm15, %ymm1");
  asm volatile ("vpor %xmm2, %xmm15, %xmm2");
  asm volatile ("vpor %ymm2, %ymm1, %ymm15");
  return 0; /* end vpor_xor_test  */
}

int
vpcmpeq_test ()
{
  /* start vpcmpeq_test.  */
  /* Using GDB, load these values onto registers for testing.
     ymm0.v2_int128  = {0x0, 0x12345}
     ymm1.v8_int32 = {0xcafe, 0xbeef, 0xff, 0x1234, 0x0, 0xff00, 0xff0000ff, 0xface0f0f}
     ymm2.v8_int32 = {0xcafe0, 0xbeef, 0xff00, 0x12345678, 0x90abcdef, 0xffff00, 0xff, 0xf}
     ymm15.v2_int128 = {0xcafeface, 0xcafeface}
     this way it's easy to confirm we're undoing things correctly.  */

  /* Test all the vpcmpeq variants on a low register (number 0).  */
  asm volatile ("vpcmpeqb %xmm1, %xmm2, %xmm0");
  asm volatile ("vpcmpeqw %xmm1, %xmm2, %xmm0");
  asm volatile ("vpcmpeqd %xmm1, %xmm2, %xmm0");

  asm volatile ("vpcmpeqb %ymm1, %ymm2, %ymm0");
  asm volatile ("vpcmpeqw %ymm1, %ymm2, %ymm0");
  asm volatile ("vpcmpeqd %ymm1, %ymm2, %ymm0");

  /* Test all the vpcmpeq variants on a high register (number 15).  */
  asm volatile ("vpcmpeqb %xmm1, %xmm2, %xmm15");
  asm volatile ("vpcmpeqw %xmm1, %xmm2, %xmm15");
  asm volatile ("vpcmpeqd %xmm1, %xmm2, %xmm15");

  asm volatile ("vpcmpeqb %ymm1, %ymm2, %ymm15");
  asm volatile ("vpcmpeqw %ymm1, %ymm2, %ymm15");
  asm volatile ("vpcmpeqd %ymm1, %ymm2, %ymm15");
  return 0; /* end vpcmpeq_test  */
}

int
vpmovmskb_test ()
{
  /* start vpmovmskb_test.  */
  /* Using GDB, load these values onto registers for testing.
     rbx = 2
     r8  = 3
     r9  = 4
     this way it's easy to confirm we're undoing things correctly.  */
  asm volatile ("vpmovmskb %ymm0, %eax");
  asm volatile ("vpmovmskb %ymm0, %ebx");

  asm volatile ("vpmovmskb %ymm0, %r8");
  asm volatile ("vpmovmskb %ymm0, %r9");
  return 0; /* end vpmovmskb_test  */
}

/* This include is used to allocate the dynamic buffer and have
   the pointers aligned to a 32-bit boundary, so we can test instructions
   that require aligned memory.  */
#include "precise-aligned-alloc.c"

int
main ()
{
  dyn_buf0 = (char *) precise_aligned_alloc(32, sizeof(char) * 32, NULL);
  dyn_buf1 = (char *) precise_aligned_alloc(32, sizeof(char) * 32, NULL);
  for (int i =0; i < 32; i++)
    {
      dyn_buf0[i] = 0x20 + (i % 16);
      dyn_buf1[i] = 0;
    }
  /* Zero relevant xmm registers, se we know what to look for.  */
  asm volatile ("vmovq %0, %%xmm0": : "m" (global_buf1));
  asm volatile ("vmovq %0, %%xmm1": : "m" (global_buf1));
  asm volatile ("vmovq %0, %%xmm2": : "m" (global_buf1));
  asm volatile ("vmovq %0, %%xmm15": : "m" (global_buf1));

  vmov_test ();
  vpunpck_test ();
  vpbroadcast_test ();
  vzeroupper_test ();
  vpor_xor_test ();
  vpcmpeq_test ();
  vpmovmskb_test ();
  return 0;	/* end of main */
}
