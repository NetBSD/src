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

#define _GNU_SOURCE
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <assert.h>
#include <errno.h>
#include <asm/prctl.h>

int
main (int argc, char **argv)
{
  int i;
  int* pi = &i;
  int* pi_tagged;

  /* Enable LAM 57.  */
  errno = 0;
  syscall (SYS_arch_prctl, ARCH_ENABLE_TAGGED_ADDR, 6);
  assert_perror (errno);

  /* Add tagging at bit 61.  */
  pi_tagged = (int *) ((uintptr_t) pi | (1LL << 60));

  i = 0; /* Breakpoint here.  */
  *pi = 1;
  *pi_tagged = 2;
  *pi = 3;
  *pi_tagged = 4;

  return 0;
}
