/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023-2025 Free Software Foundation, Inc.

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

  /* Zero memory regions again, so that future tests can update them
     without worry.  */
void
reset_buffers ()
{
  for (int i = 0; i < 32; i++)
    {
      global_buf1[i] = 0;
      dyn_buf1[i] = 0;
    }
}

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

  /* Testing vmov [ss|sd] instructions.  */
  /* Note, vmovss only works with XMM registers, not YMM registers,
     according to the intel manual.  Also, initializing the variables
     uses xmm0 in my machine, so we can't test with it, so use xmm1
     instead.  */

  /* Move single precision floats to and from memory.  */
  float f1 = 1.5, f2 = 4.2;
  asm volatile ("vmovss %0, %%xmm1" : : "m"(f1));
  asm volatile ("vmovss %0, %%xmm15": : "m"(f2));
  asm volatile ("vmovss %%xmm1, %0" :  "=m"(f2));
  asm volatile ("vmovss %%xmm15, %0":  "=m"(f1));

  asm volatile ("vmovss %xmm15, %xmm1, %xmm2");
  asm volatile ("vmovss %xmm15, %xmm1, %xmm8");
  asm volatile ("vmovss %xmm1, %xmm2, %xmm15");
  asm volatile ("vmovss %xmm2, %xmm15, %xmm1");

  /* Testing double precision floats.  */
  double d1 = -1.5, d2 = -2.5;
  asm volatile ("vmovsd %0, %%xmm1" : : "m"(d1));
  asm volatile ("vmovsd %0, %%xmm15": : "m"(d2));
  asm volatile ("vmovsd %%xmm1, %0" :  "=m"(d2));
  asm volatile ("vmovsd %%xmm15, %0":  "=m"(d1));

  asm volatile ("vmovsd %xmm15, %xmm1, %xmm2");
  asm volatile ("vmovsd %xmm15, %xmm1, %xmm8");
  asm volatile ("vmovsd %xmm1, %xmm2, %xmm15");
  asm volatile ("vmovsd %xmm2, %xmm15, %xmm1");

  /* "reset" all the buffers.  This doesn't zero them all, but
     it zeroes the start which lets us ensure the tests see
     some changes.  */
  asm volatile ("vmovq %%xmm3, %0": "=m" (buf1));
  asm volatile ("vmovq %%xmm3, %0": "=m" (global_buf1));
  asm volatile ("vmovq %%xmm3, %0": "=m" (*dyn_buf1));

  /* Testing vmovu[ps|pd] instructions.  Even though there are aligned
     versions of these instructions like vmovdq[u|a], they have different
     opcodes, meaning they'll need to be tested separately.  */

  asm volatile ("vmovups %0, %%xmm0"  : : "m"(buf0));
  asm volatile ("vmovupd %0, %%ymm15" : : "m"(buf1));
  asm volatile ("vmovupd %%xmm0, %0"  : : "m"(buf1));
  asm volatile ("vmovups %%ymm15, %0" : : "m"(buf1));

  asm volatile ("vmovups %0, %%xmm0"  : : "m"(global_buf0));
  asm volatile ("vmovupd %0, %%ymm15" : : "m"(global_buf1));
  asm volatile ("vmovupd %%xmm0, %0"  : : "m"(global_buf1));
  asm volatile ("vmovups %%ymm15, %0" : : "m"(global_buf1));

  asm volatile ("vmovups %0, %%xmm0"  : : "m"(*dyn_buf0));
  asm volatile ("vmovupd %0, %%ymm15" : : "m"(*dyn_buf1));
  asm volatile ("vmovupd %%xmm0, %0"  : : "m"(*dyn_buf1));
  asm volatile ("vmovups %%ymm15, %0" : : "m"(*dyn_buf1));

  asm volatile ("vmovaps %0, %%xmm0"  : : "m"(*dyn_buf0));
  asm volatile ("vmovapd %0, %%ymm15" : : "m"(*dyn_buf1));
  asm volatile ("vmovapd %%xmm0, %0"  : : "m"(*dyn_buf1));
  asm volatile ("vmovaps %%ymm15, %0" : : "m"(*dyn_buf1));

  /* Testing vmov[hl|lh]ps and vmov[h|l]pd.  */
  asm volatile ("vmovhlps %xmm1, %xmm8, %xmm0");
  asm volatile ("vmovhlps %xmm1, %xmm2, %xmm15");
  asm volatile ("vmovlhps %xmm1, %xmm8, %xmm0");
  asm volatile ("vmovlhps %xmm1, %xmm2, %xmm15");

  asm volatile ("vmovhps %0, %%xmm1, %%xmm0" : : "m"(buf0));
  asm volatile ("vmovhps %%xmm0, %0" : "=m" (buf1));
  asm volatile ("vmovhpd %0, %%xmm1, %%xmm15" : : "m"(global_buf0));
  asm volatile ("vmovhpd %%xmm15, %0" : "=m" (global_buf1));
  asm volatile ("vmovlpd %0, %%xmm1, %%xmm15" : : "m"(*dyn_buf0));
  asm volatile ("vmovlpd %%xmm15, %0" : "=m" (*dyn_buf1));

  asm volatile ("vmovddup %xmm1, %xmm15");
  asm volatile ("vmovddup %ymm2, %ymm0");

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
     ymm2.v2_int128 = {0x0, 0x0}
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

  /* Test some of the floating point unpack instructions.  */
  /* 17 27 16 26 15 25 14 24 ...*/
  asm volatile ("vunpcklps  %xmm0, %xmm1, %xmm15");
  /* 17 16 27 26 15 14 25 24 ...*/
  asm volatile ("vunpcklps  %ymm0, %ymm1, %ymm2");
  /* 17 16 15 14 27 26 25 24 ...*/
  asm volatile ("vunpcklpd  %xmm0, %xmm1,  %xmm2");
  /* 17 16 15 14 13 12 11 10 ...*/
  asm volatile ("vunpcklpd %ymm0, %ymm1, %ymm15");
  /* 17 27 16 26 15 25 14 24 ...*/
  asm volatile ("vunpckhps  %xmm0, %xmm1, %xmm15");
  /* 17 16 27 26 15 14 25 24 ...*/
  asm volatile ("vunpckhps  %ymm0, %ymm1, %ymm2");
  /* 17 16 15 14 27 26 25 24 ...*/
  asm volatile ("vunpckhpd  %xmm0, %xmm1,  %xmm2");
  /* 17 16 15 14 13 12 11 10 ...*/
  asm volatile ("vunpckhpd %ymm0, %ymm1, %ymm15");

  /* We have a return statement to deal with
     epilogue in different compilers.  */
  return 0; /* end vpunpck_test */
}

/* Test if we can record vpbroadcast and vbroadcast instructions.  */
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

  asm volatile ("vbroadcastss %xmm1, %xmm0");
  asm volatile ("vbroadcastss %xmm1, %ymm15");
  asm volatile ("vbroadcastss %0, %%ymm0" : : "m" (global_buf0));
  asm volatile ("vbroadcastss %0, %%xmm15": : "m" (*dyn_buf0));
  asm volatile ("vbroadcastsd %xmm1, %ymm0");
  asm volatile ("vbroadcastsd %0, %%ymm15": : "m" (global_buf0));
  asm volatile ("vbroadcastf128 %0, %%ymm0" : : "m" (*dyn_buf0));

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

/* Test record arithmetic instructions.  */
int
arith_test ()
{
  /* start arith_test.  */
  /* Using GDB, load these values onto registers for testing.
     ymm0.v8_float = {0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5}
     ymm1.v8_float = {0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5}
     ymm2.v2_int128 = {0x0, 0x0}
     ymm15.v2_int128 = {0x0, 0x0}
     this way it's easy to confirm we're undoing things correctly.  */
  asm volatile ("vaddps %xmm0, %xmm1, %xmm15");
  asm volatile ("vaddps %ymm0, %ymm1, %ymm15");
  asm volatile ("vaddpd %xmm0, %xmm1, %xmm15");
  asm volatile ("vaddpd %ymm0, %ymm1, %ymm15");
  asm volatile ("vaddss %xmm0, %xmm1, %xmm15");
  asm volatile ("vaddsd %xmm0, %xmm1, %xmm15");

  asm volatile ("vmulps %xmm0, %xmm1, %xmm15");
  asm volatile ("vmulps %ymm0, %ymm1, %ymm15");
  asm volatile ("vmulpd %xmm0, %xmm1, %xmm15");
  asm volatile ("vmulpd %ymm0, %ymm1, %ymm15");
  asm volatile ("vmulss %xmm0, %xmm1, %xmm15");
  asm volatile ("vmulsd %xmm0, %xmm1, %xmm15");

  asm volatile ("vsubps %xmm0, %xmm1, %xmm15");
  asm volatile ("vsubps %ymm0, %ymm1, %ymm15");
  asm volatile ("vsubpd %xmm0, %xmm1, %xmm15");
  asm volatile ("vsubpd %ymm0, %ymm1, %ymm15");
  asm volatile ("vsubss %xmm0, %xmm1, %xmm15");
  asm volatile ("vsubsd %xmm0, %xmm1, %xmm15");

  asm volatile ("vdivps %xmm0, %xmm1, %xmm15");
  asm volatile ("vdivps %ymm0, %ymm1, %ymm15");
  asm volatile ("vdivpd %xmm0, %xmm1, %xmm15");
  asm volatile ("vdivpd %ymm0, %ymm1, %ymm15");
  asm volatile ("vdivss %xmm0, %xmm1, %xmm15");
  asm volatile ("vdivsd %xmm0, %xmm1, %xmm15");

  asm volatile ("vminps %xmm0, %xmm1, %xmm15");
  asm volatile ("vminps %ymm0, %ymm1, %ymm15");
  asm volatile ("vminpd %xmm0, %xmm1, %xmm15");
  asm volatile ("vminpd %ymm0, %ymm1, %ymm15");
  asm volatile ("vminss %xmm0, %xmm1, %xmm15");
  asm volatile ("vminsd %xmm0, %xmm1, %xmm15");

  asm volatile ("vmaxps %xmm0, %xmm1, %xmm15");
  asm volatile ("vmaxps %ymm0, %ymm1, %ymm15");
  asm volatile ("vmaxpd %xmm0, %xmm1, %xmm15");
  asm volatile ("vmaxpd %ymm0, %ymm1, %ymm15");
  asm volatile ("vmaxss %xmm0, %xmm1, %xmm15");
  asm volatile ("vmaxsd %xmm0, %xmm1, %xmm15");

  /* Some sanity checks for other arithmetic instructions.  */
  asm volatile ("vpaddb %xmm0, %xmm1, %xmm2");
  asm volatile ("vpaddw %xmm0, %xmm1, %xmm15");
  asm volatile ("vpaddd %ymm0, %ymm1, %ymm2");
  asm volatile ("vpaddq %ymm0, %ymm1, %ymm15");

  asm volatile ("vpmullw %xmm0, %xmm1, %xmm2");
  asm volatile ("vpmulld %xmm0, %xmm1, %xmm15");
  asm volatile ("vpmulhw %ymm0, %ymm1, %ymm2");
  asm volatile ("vpmulhuw %ymm0, %ymm1, %ymm15");
  asm volatile ("vpmuludq %ymm0, %ymm1, %ymm15");

  asm volatile ("vxorps %xmm0, %xmm1, %xmm2");
  asm volatile ("vxorpd %ymm0, %ymm1, %ymm2");
  asm volatile ("vpand %xmm0, %xmm1, %xmm15");
  asm volatile ("vpandn %ymm0, %ymm1, %ymm15");

  asm volatile ("vpsadbw %xmm0, %xmm1, %xmm2");
  asm volatile ("vpsadbw %ymm0, %ymm1, %ymm15");

  return 0; /* end arith_test  */
}

int
vaddsubpd_test ()
{
  /* start vaddsubpd_test */
  /* YMM test.  */
  asm volatile ("vaddsubpd %ymm15,%ymm1,%ymm0");
  asm volatile ("vaddsubpd %ymm0,%ymm1,%ymm15");
  asm volatile ("vaddsubpd %ymm2,%ymm3,%ymm4");

  /* XMM test.  */
  asm volatile ("vaddsubpd %xmm15,%xmm1,%xmm2");
  asm volatile ("vaddsubpd %xmm0,%xmm1,%xmm10");
  return 0; /* end vaddsubpd_test */
}

int
vaddsubps_test ()
{
  /* start vaddsubps_test */
  /* YMM test.  */
  asm volatile ("vaddsubps %ymm15,%ymm1,%ymm2");
  asm volatile ("vaddsubps %ymm0,%ymm1,%ymm10");
  asm volatile ("vaddsubps %ymm2,%ymm3,%ymm4");

  /* XMM test.  */
  asm volatile ("vaddsubps %xmm0,%xmm1,%xmm15");
  asm volatile ("vaddsubps %xmm15,%xmm1,%xmm0");
  return 0; /* end vaddsubps_test */
}


/* Test record shifting instructions.  */
int
shift_test ()
{
  /* start shift_test.  */
  /* Using GDB, load these values onto registers for testing.
     ymm0.v2_int128 = {0, 0}
     ymm1.v16_int16 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}
     xmm2.uint128 = 1
     ymm15.v2_int128 = {0x0, 0x0}
     this way it's easy to confirm we're undoing things correctly.  */

  asm volatile ("vpsllw $1, %xmm1, %xmm0");
  asm volatile ("vpsllw %xmm2, %ymm1, %ymm0");
  asm volatile ("vpslld $3, %ymm1, %ymm15");
  asm volatile ("vpslld %xmm2, %xmm1, %xmm15");
  asm volatile ("vpsllq $5, %xmm1, %xmm15");
  asm volatile ("vpsllq %xmm2, %ymm1, %ymm15");

  asm volatile ("vpsraw $1, %xmm1, %xmm0");
  asm volatile ("vpsraw %xmm2, %ymm1, %ymm0");
  asm volatile ("vpsrad $3, %ymm1, %ymm15");
  asm volatile ("vpsrad %xmm2, %xmm1, %xmm15");

  asm volatile ("vpsrlw $1, %xmm1, %xmm0");
  asm volatile ("vpsrlw %xmm2, %ymm1, %ymm0");
  asm volatile ("vpsrld $3, %ymm1, %ymm15");
  asm volatile ("vpsrld %xmm2, %xmm1, %xmm15");
  asm volatile ("vpsrlq $5, %xmm1, %xmm15");
  asm volatile ("vpsrlq %xmm2, %ymm1, %ymm15");

  /* The dq version is treated separately in the manual, so
     we test it separately just to be sure.  */
  asm volatile ("vpslldq $1, %xmm1, %xmm0");
  asm volatile ("vpslldq $2, %ymm1, %ymm0");
  asm volatile ("vpslldq $3, %xmm1, %xmm15");
  asm volatile ("vpslldq $4, %ymm1, %ymm15");

  asm volatile ("vpsrldq $1, %xmm1, %xmm0");
  asm volatile ("vpsrldq $2, %ymm1, %ymm0");
  asm volatile ("vpsrldq $3, %xmm1, %xmm15");
  asm volatile ("vpsrldq $4, %ymm1, %ymm15");

  return 0; /* end shift_test  */
}

int
shuffle_test ()
{
  /* start shuffle_test.  */
  /* Using GDB, load these values onto registers for testing.
     ymm0.v2_int128 = {0, 0}
     ymm1.v16_int16 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}
     ymm2.v16_int15 = {17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32}
     ymm15.v2_int128 = {0x0, 0x0}
     this way it's easy to confirm we're undoing things correctly.  */

  asm volatile ("vpshufb %xmm1, %xmm2, %xmm0");
  asm volatile ("vpshufb %ymm1, %ymm2, %ymm15");
  asm volatile ("vpshufd $1, %ymm2, %ymm0");
  asm volatile ("vpshufd $2, %xmm2, %xmm15");

  asm volatile ("vpshufhw $3, %xmm2, %xmm0");
  asm volatile ("vpshufhw $4, %ymm2, %ymm15");
  asm volatile ("vpshuflw $5, %ymm2, %ymm0");
  asm volatile ("vpshuflw $6, %xmm2, %xmm15");

  asm volatile ("vshufps $1, %xmm1, %xmm2, %xmm0");
  asm volatile ("vshufps $2, %ymm1, %ymm2, %ymm15");
  asm volatile ("vshufpd $4, %ymm1, %ymm2, %ymm0");
  asm volatile ("vshufpd $8, %xmm1, %xmm2, %xmm15");

  return 0; /* end shuffle_test  */
}

int
permute_test ()
{
  /* start permute_test.  */
  /* Using GDB, load these values onto registers for testing.
     ymm0.v2_int128 = {0, 0}
     ymm1.v16_int16 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}
     ymm2.v16_int16 = {17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32}
     ymm15.v2_int128 = {0x0, 0x0}
     eax = 0
     this way it's easy to confirm we're undoing things correctly.  */
  asm volatile ("vperm2f128 $1, %ymm1, %ymm2, %ymm0");
  asm volatile ("vperm2f128 $0, %ymm1, %ymm2, %ymm15");
  asm volatile ("vperm2i128 $1, %ymm2, %ymm1, %ymm0");
  asm volatile ("vperm2i128 $0, %ymm2, %ymm1, %ymm15");

  asm volatile ("vpermd %ymm1, %ymm2, %ymm0");
  asm volatile ("vpermd %ymm1, %ymm2, %ymm15");
  asm volatile ("vpermq $1, %ymm1, %ymm0");
  asm volatile ("vpermq $0, %ymm2, %ymm15");

  asm volatile ("vpermilpd %ymm1, %ymm2, %ymm0");
  asm volatile ("vpermilpd %xmm1, %xmm2, %xmm15");
  asm volatile ("vpermilpd $1, %ymm2, %ymm15");
  asm volatile ("vpermilpd $0, %xmm2, %xmm0");
  asm volatile ("vpermilps %ymm1, %ymm2, %ymm0");
  asm volatile ("vpermilps %xmm1, %xmm2, %xmm15");
  asm volatile ("vpermilps $1, %ymm2, %ymm15");
  asm volatile ("vpermilps $0, %xmm2, %xmm0");

  asm volatile ("vpermpd $0, %ymm1, %ymm15");
  asm volatile ("vpermpd $0, %ymm2, %ymm0");
  asm volatile ("vpermps %ymm1, %ymm2, %ymm0");
  asm volatile ("vpermps %ymm1, %ymm2, %ymm15");

  return 0; /* end permute_test  */
}

int
extract_insert_test ()
{
  /* start extract_insert_test.  */
  /* Using GDB, load these values onto registers for testing.
     ymm0.v2_int128 = {0, 0}
     ymm1.v16_int16 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}
     xmm2.uint128 = 0xcafe
     ymm15.v2_int128 = {0x0, 0x0}
     eax = 0
     this way it's easy to confirm we're undoing things correctly.  */

  asm volatile ("vinserti128 $1, %xmm2, %ymm1, %ymm0");
  asm volatile ("vinsertf128 $0, %xmm2, %ymm1, %ymm15");
  asm volatile ("vextracti128 $1, %ymm1, %xmm0");
  asm volatile ("vextractf128 $0, %ymm1, %xmm15");
  asm volatile ("vinsertps $16, %xmm2, %xmm1, %xmm0");
  asm volatile ("vextractps $0, %xmm2, %rax");

  asm volatile ("vpextrb $5, %xmm1, %rax");
  asm volatile ("vpextrb $4, %%xmm1, %0" : "=m" (global_buf1));
  asm volatile ("vpextrd $3, %xmm1, %eax");
  asm volatile ("vpextrd $2, %%xmm1, %0" : "=m" (global_buf1));
  asm volatile ("vpextrq $1, %xmm1, %rax");
  asm volatile ("vpextrq $0, %%xmm1, %0" : "=m" (global_buf1));

  asm volatile ("vpinsrb $3, %rax, %xmm2, %xmm0");
  asm volatile ("vpinsrw $2, %eax, %xmm2, %xmm15");
  asm volatile ("vpinsrd $1, %eax, %xmm2, %xmm0");
  asm volatile ("vpinsrq $0, %rax, %xmm2, %xmm15");

  /* vpextrw has completely different mechanics to other vpextr
     instructions, so separate them for ease of testing later.  */
  asm volatile ("vpextrw $1, %xmm1, %eax");
  asm volatile ("vpextrw $1, %%xmm1, %0" : "=m" (global_buf1));

  return 0; /* end extract_insert_test  */
}

int
blend_test ()
{
  /* start blend_test.  */
  /* Using GDB, load these values onto registers for testing.
     ymm0.v2_int128 = {0, 0}
     ymm1.v16_int16 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}
     ymm2.v16_int16 = {17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32}
     ymm15.v2_int128 = {0x0, 0x0}
     this way it's easy to confirm we're undoing things correctly.  */

  asm volatile ("vblendps $5, %xmm1, %xmm2, %xmm0");
  asm volatile ("vblendpd $10, %ymm1, %ymm2, %ymm15");
  asm volatile ("vblendvps %ymm15, %ymm1, %ymm2, %ymm0");
  asm volatile ("vblendvpd %xmm0, %xmm1, %xmm2, %xmm15");

  asm volatile ("vpblendw $94, %ymm1, %ymm2, %ymm15");
  asm volatile ("vpblendw $47, %xmm1, %xmm2, %xmm0");
  asm volatile ("vpblendd $22, %ymm1, %ymm2, %ymm0");
  asm volatile ("vpblendd $11, %xmm1, %xmm2, %xmm15");
  asm volatile ("vpblendvb %xmm0, %xmm1, %xmm2, %xmm15");
  asm volatile ("vpblendvb %ymm0, %ymm1, %ymm2, %ymm0");

  return 0; /* end blend_test  */
}

int
compare_test ()
{
  /* start compare_test.  */
  /* Using GDB, load these values onto registers for testing.
     xmm0.v4_float = {0, 1.5, 2, 0}
     xmm1.v4_float = {0, 1, 2.5, -1}
     xmm15.v4_float = {-1, -2, 10, 100}
     eflags = 2
     eflags can't be set to some values, if we set it to 0, it'll
     be reset to 2, so set it to that directly to make results less
     confusing.
     this way it's easy to confirm we're undoing things correctly.  */

  asm volatile ("vcomisd %xmm0, %xmm1");
  asm volatile ("vcomiss %xmm15, %xmm1");
  asm volatile ("vucomiss %xmm1, %xmm15");
  asm volatile ("vucomisd %xmm15, %xmm0");

  return 0; /* end compare_test  */
}

int
pack_test ()
{
  /* start pack_test.  */
  /* Using GDB, load these values onto registers for testing.
     xmm0.v4_float = {0, 1.5, 2, 0}
     xmm1.v4_float = {0, 1, 2.5, -1}
     xmm2.v4_float = {0, 1, 2.5, -1}
     xmm15.v4_float = {-1, -2, 10, 100}
     this way it's easy to confirm we're undoing things correctly.  */

  asm volatile ("vpacksswb %xmm1, %xmm2, %xmm0");
  asm volatile ("vpacksswb %ymm1, %ymm2, %ymm15");
  asm volatile ("vpackssdw %xmm1, %xmm2, %xmm15");
  asm volatile ("vpackssdw %ymm1, %ymm2, %ymm0");
  asm volatile ("vpackuswb %xmm1, %xmm2, %xmm0");
  asm volatile ("vpackuswb %ymm1, %ymm2, %ymm15");
  asm volatile ("vpackusdw %xmm1, %xmm2, %xmm15");
  asm volatile ("vpackusdw %ymm1, %ymm2, %ymm0");

  return 0; /* end pack_test  */
}

int
convert_test ()
{
  /* start convert_test.  */
  /* Using GDB, load these values onto registers for testing.
     xmm0.v2_int128 = {0, 0}
     xmm1.v4_float = {0, 1, 2.5, 10}
     xmm15.v2_int128 = {0, 0}
     ecx = -1
     ebx = 0
     this way it's easy to confirm we're undoing things correctly.  */

  asm volatile ("vcvtdq2ps %xmm1, %xmm0");
  asm volatile ("vcvtdq2pd %xmm1, %xmm15");

  asm volatile ("vcvtps2dq %xmm1, %xmm15");
  asm volatile ("vcvtps2pd %xmm1, %xmm0");
  asm volatile ("vcvtpd2ps %xmm1, %xmm15");
  asm volatile ("vcvtpd2dq %xmm1, %xmm0");

  asm volatile ("vcvtsd2si %xmm1, %rbx");
  asm volatile ("vcvtsd2ss %xmm0, %xmm1, %xmm15");
  asm volatile ("vcvtsi2sd %rcx, %xmm1, %xmm0");
  asm volatile ("vcvtsi2ss %rcx, %xmm1, %xmm15");
  asm volatile ("vcvtss2sd %xmm15, %xmm1, %xmm0");
  asm volatile ("vcvtss2si %xmm1, %rbx");

  asm volatile ("vcvttpd2dq %xmm1, %xmm0");
  asm volatile ("vcvttps2dq %xmm1, %xmm15");
  asm volatile ("vcvttsd2si %xmm0, %rbx");
  asm volatile ("vcvttss2si %xmm1, %ecx");

  return 0; /* end convert_test  */
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
  asm volatile ("vmovq %0, %%xmm3": : "m" (global_buf1));
  asm volatile ("vmovq %0, %%xmm15": : "m" (global_buf1));

  vmov_test ();
  reset_buffers ();
  vpunpck_test ();
  vpbroadcast_test ();
  vzeroupper_test ();
  vpor_xor_test ();
  vpcmpeq_test ();
  vpmovmskb_test ();
  arith_test ();
  vaddsubpd_test ();
  vaddsubps_test ();
  shift_test ();
  shuffle_test ();
  permute_test ();
  extract_insert_test ();
  blend_test ();
  compare_test ();
  pack_test ();
  convert_test ();
  return 0;	/* end of main */
}
