/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017 Free Software Foundation, Inc.

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

#include <cpuid.h>
#include <stdint.h>

/* 0 if the CPU supports rdrand and non-zero otherwise.  */
static unsigned int supports_rdrand;

/* 0 if the CPU supports rdseed and non-zero otherwise.  */
static unsigned int supports_rdseed;

/* Check supported features and set globals accordingly.  The globals
   can be used to prevent unsupported tests from running.  */

static void
check_supported_features (void)
{
  unsigned int rdrand_mask = (1 << 30);
  unsigned int rdseed_mask = (1 << 18);
  unsigned int eax, ebx, ecx, edx;
  unsigned int vendor;
  unsigned int max_level;

  max_level = __get_cpuid_max (0, &vendor);

  if (max_level < 1)
    return;

  __cpuid (1, eax, ebx, ecx, edx);

  supports_rdrand = ((ecx & rdrand_mask) == rdrand_mask);

  if (max_level >= 7)
    {
      __cpuid_count (7, 0, eax, ebx, ecx, edx);
      supports_rdseed = ((ebx & rdseed_mask) == rdseed_mask);
    }
}

/* Test rdrand support for various output registers.  */

void
rdrand (void)
{
  /* Get a random number from the rdrand assembly instruction.  */
  register uint64_t number;

  if (!supports_rdrand)
    return;

  /* 16-bit random numbers.  */
  __asm__ volatile ("rdrand %%ax;" : "=r" (number));
  __asm__ volatile ("rdrand %%bx;" : "=r" (number));
  __asm__ volatile ("rdrand %%cx;" : "=r" (number));
  __asm__ volatile ("rdrand %%dx;" : "=r" (number));

  __asm__ volatile ("mov %%di, %%ax;" : "=r" (number));
  __asm__ volatile ("rdrand %%di;" : "=r" (number));
  __asm__ volatile ("mov %%ax, %%di;" : "=r" (number));

  __asm__ volatile ("mov %%si, %%ax;" : "=r" (number));
  __asm__ volatile ("rdrand %%si;" : "=r" (number));
  __asm__ volatile ("mov %%ax, %%si;" : "=r" (number));

  __asm__ volatile ("mov %%bp, %%ax;" : "=r" (number));
  __asm__ volatile ("rdrand %%bp;" : "=r" (number));
  __asm__ volatile ("mov %%ax, %%bp;" : "=r" (number));

  __asm__ volatile ("mov %%sp, %%ax;" : "=r" (number));
  __asm__ volatile ("rdrand %%sp;" : "=r" (number));
  __asm__ volatile ("mov %%ax, %%sp;" : "=r" (number));

  __asm__ volatile ("rdrand %%r8w;" : "=r" (number));
  __asm__ volatile ("rdrand %%r9w;" : "=r" (number));
  __asm__ volatile ("rdrand %%r10w;" : "=r" (number));
  __asm__ volatile ("rdrand %%r11w;" : "=r" (number));
  __asm__ volatile ("rdrand %%r12w;" : "=r" (number));
  __asm__ volatile ("rdrand %%r13w;" : "=r" (number));
  __asm__ volatile ("rdrand %%r14w;" : "=r" (number));
  __asm__ volatile ("rdrand %%r15w;" : "=r" (number));

  /* 32-bit random numbers.  */
  __asm__ volatile ("rdrand %%eax;" : "=r" (number));
  __asm__ volatile ("rdrand %%ebx;" : "=r" (number));
  __asm__ volatile ("rdrand %%ecx;" : "=r" (number));
  __asm__ volatile ("rdrand %%edx;" : "=r" (number));

  __asm__ volatile ("mov %%rdi, %%rax;" : "=r" (number));
  __asm__ volatile ("rdrand %%edi;" : "=r" (number));
  __asm__ volatile ("mov %%rax, %%rdi;" : "=r" (number));

  __asm__ volatile ("mov %%rsi, %%rax;" : "=r" (number));
  __asm__ volatile ("rdrand %%esi;" : "=r" (number));
  __asm__ volatile ("mov %%rax, %%rsi;" : "=r" (number));

  __asm__ volatile ("mov %%rbp, %%rax;" : "=r" (number));
  __asm__ volatile ("rdrand %%ebp;" : "=r" (number));
  __asm__ volatile ("mov %%rax, %%rbp;" : "=r" (number));

  __asm__ volatile ("mov %%rsp, %%rax;" : "=r" (number));
  __asm__ volatile ("rdrand %%esp;" : "=r" (number));
  __asm__ volatile ("mov %%rax, %%rsp;" : "=r" (number));

  __asm__ volatile ("rdrand %%r8d;" : "=r" (number));
  __asm__ volatile ("rdrand %%r9d;" : "=r" (number));
  __asm__ volatile ("rdrand %%r10d;" : "=r" (number));
  __asm__ volatile ("rdrand %%r11d;" : "=r" (number));
  __asm__ volatile ("rdrand %%r12d;" : "=r" (number));
  __asm__ volatile ("rdrand %%r13d;" : "=r" (number));
  __asm__ volatile ("rdrand %%r14d;" : "=r" (number));
  __asm__ volatile ("rdrand %%r15d;" : "=r" (number));

  /* 64-bit random numbers.  */
  __asm__ volatile ("rdrand %%rax;" : "=r" (number));
  __asm__ volatile ("rdrand %%rbx;" : "=r" (number));
  __asm__ volatile ("rdrand %%rcx;" : "=r" (number));
  __asm__ volatile ("rdrand %%rdx;" : "=r" (number));

  __asm__ volatile ("mov %%rdi, %%rax;" : "=r" (number));
  __asm__ volatile ("rdrand %%rdi;" : "=r" (number));
  __asm__ volatile ("mov %%rax, %%rdi;" : "=r" (number));

  __asm__ volatile ("mov %%rsi, %%rax;" : "=r" (number));
  __asm__ volatile ("rdrand %%rsi;" : "=r" (number));
  __asm__ volatile ("mov %%rax, %%rsi;" : "=r" (number));

  __asm__ volatile ("mov %%rbp, %%rax;" : "=r" (number));
  __asm__ volatile ("rdrand %%rbp;" : "=r" (number));
  __asm__ volatile ("mov %%rax, %%rbp;" : "=r" (number));

  __asm__ volatile ("mov %%rsp, %%rax;" : "=r" (number));
  __asm__ volatile ("rdrand %%rsp;" : "=r" (number));
  __asm__ volatile ("mov %%rax, %%rsp;" : "=r" (number));

  __asm__ volatile ("rdrand %%r8;" : "=r" (number));
  __asm__ volatile ("rdrand %%r9;" : "=r" (number));
  __asm__ volatile ("rdrand %%r10;" : "=r" (number));
  __asm__ volatile ("rdrand %%r11;" : "=r" (number));
  __asm__ volatile ("rdrand %%r12;" : "=r" (number));
  __asm__ volatile ("rdrand %%r13;" : "=r" (number));
  __asm__ volatile ("rdrand %%r14;" : "=r" (number));
  __asm__ volatile ("rdrand %%r15;" : "=r" (number));
}

/* Test rdseed support for various output registers.  */

void
rdseed (void)
{
  /* Get a random seed from the rdseed assembly instruction.  */
  register long seed;

  if (!supports_rdseed)
    return;

  /* 16-bit random seeds.  */
  __asm__ volatile ("rdseed %%ax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%bx;" : "=r" (seed));
  __asm__ volatile ("rdseed %%cx;" : "=r" (seed));
  __asm__ volatile ("rdseed %%dx;" : "=r" (seed));

  __asm__ volatile ("mov %%di, %%ax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%di;" : "=r" (seed));
  __asm__ volatile ("mov %%ax, %%di;" : "=r" (seed));

  __asm__ volatile ("mov %%si, %%ax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%si;" : "=r" (seed));
  __asm__ volatile ("mov %%ax, %%si;" : "=r" (seed));

  __asm__ volatile ("mov %%bp, %%ax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%bp;" : "=r" (seed));
  __asm__ volatile ("mov %%ax, %%bp;" : "=r" (seed));

  __asm__ volatile ("mov %%sp, %%ax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%sp;" : "=r" (seed));
  __asm__ volatile ("mov %%ax, %%sp;" : "=r" (seed));

  __asm__ volatile ("rdseed %%r8w;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r9w;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r10w;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r11w;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r12w;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r13w;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r14w;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r15w;" : "=r" (seed));

  /* 32-bit random seeds.  */
  __asm__ volatile ("rdseed %%eax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%ebx;" : "=r" (seed));
  __asm__ volatile ("rdseed %%ecx;" : "=r" (seed));
  __asm__ volatile ("rdseed %%edx;" : "=r" (seed));

  __asm__ volatile ("mov %%rdi, %%rax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%edi;" : "=r" (seed));
  __asm__ volatile ("mov %%rax, %%rdi;" : "=r" (seed));

  __asm__ volatile ("mov %%rsi, %%rax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%esi;" : "=r" (seed));
  __asm__ volatile ("mov %%rax, %%rsi;" : "=r" (seed));

  __asm__ volatile ("mov %%rbp, %%rax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%ebp;" : "=r" (seed));
  __asm__ volatile ("mov %%rax, %%rbp;" : "=r" (seed));

  __asm__ volatile ("mov %%rsp, %%rax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%esp;" : "=r" (seed));
  __asm__ volatile ("mov %%rax, %%rsp;" : "=r" (seed));

  __asm__ volatile ("rdseed %%r8d;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r9d;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r10d;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r11d;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r12d;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r13d;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r14d;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r15d;" : "=r" (seed));

  /* 64-bit random seeds.  */
  __asm__ volatile ("rdseed %%rax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%rbx;" : "=r" (seed));
  __asm__ volatile ("rdseed %%rcx;" : "=r" (seed));
  __asm__ volatile ("rdseed %%rdx;" : "=r" (seed));

  __asm__ volatile ("mov %%rdi, %%rax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%rdi;" : "=r" (seed));
  __asm__ volatile ("mov %%rax, %%rdi;" : "=r" (seed));

  __asm__ volatile ("mov %%rsi, %%rax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%rsi;" : "=r" (seed));
  __asm__ volatile ("mov %%rax, %%rsi;" : "=r" (seed));

  __asm__ volatile ("mov %%rbp, %%rax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%rbp;" : "=r" (seed));
  __asm__ volatile ("mov %%rax, %%rbp;" : "=r" (seed));

  __asm__ volatile ("mov %%rsp, %%rax;" : "=r" (seed));
  __asm__ volatile ("rdseed %%rsp;" : "=r" (seed));
  __asm__ volatile ("mov %%rax, %%rsp;" : "=r" (seed));

  __asm__ volatile ("rdseed %%r8;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r9;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r10;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r11;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r12;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r13;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r14;" : "=r" (seed));
  __asm__ volatile ("rdseed %%r15;" : "=r" (seed));
}

/* Initialize arch-specific bits.  */

static void
initialize (void)
{
  /* Initialize supported features.  */
  check_supported_features ();
}

/* Functions testing instruction decodings.  GDB will test all of these.  */
static testcase_ftype testcases[] =
{
  rdrand,
  rdseed
};
