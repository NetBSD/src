/*	$NetBSD: bitstring_test.c,v 1.6 2002/02/21 07:38:14 itojun Exp $	*/

/*-
 * Copyright (c) 1993 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * this is a simple program to test bitstring.h
 * inspect the ouput, you should notice problems
 * choose the ATT or BSD flavor
 */
#if 0
#define ATT
#else
#define BSD /*-*/
#endif

/* include the following define if you want the
 * program to link. this corrects a misspeling
 * in bitstring.h
 */
#define _bitstr_size bitstr_size
 
#include <stdio.h>
#include <stdlib.h>

/* #ifdef NOTSOGOOD */
#include "bitstring.h"
/* #else */
/* #include "gbitstring.h" */
/* #endif */

int TEST_LENGTH;
#define DECL_TEST_LENGTH	37	/* a mostly random number */

main(int argc, char *argv[])
{
  void clearbits();
  void printbits();
  int b, i;
  bitstr_t *bs;
  bitstr_t bit_decl(bss, DECL_TEST_LENGTH);

  if (argc > 1)
	TEST_LENGTH = atoi(argv[1]);
  else
	TEST_LENGTH = DECL_TEST_LENGTH;

  if (TEST_LENGTH < 4) {
	fprintf(stderr, "TEST_LENGTH must be at least 4, but it is %d\n",
		TEST_LENGTH);
	exit(1);
  }

  (void) printf("Testing with TEST_LENGTH = %d\n\n", TEST_LENGTH);

  (void) printf("test _bit_byte, _bit_mask, and bitstr_size\n");
  (void) printf("  i   _bit_byte(i)   _bit_mask(i) bitstr_size(i)\n");
  for (i=0; i<TEST_LENGTH; i++) {
    (void) printf("%3d%15d%15d%15d\n",
      i, _bit_byte(i), _bit_mask(i), bitstr_size(i));
    }

  bs = bit_alloc(TEST_LENGTH);
  clearbits(bs, TEST_LENGTH);
  (void) printf("\ntest bit_alloc, clearbits, bit_ffc, bit_ffs\n");
  (void) printf("be:   0  -1 ");
  for (i=0; i < TEST_LENGTH; i++)
	(void) putchar('0');
  (void) printf("\nis: ");
  printbits(bs, TEST_LENGTH);

  (void) printf("\ntest bit_set\n");
  for (i=0; i<TEST_LENGTH; i+=3) {
    bit_set(bs, i); 
    }
  (void) printf("be:   1   0 ");
  for (i=0; i < TEST_LENGTH; i++)
	(void) putchar(*("100" + (i % 3)));
  (void) printf("\nis: ");
  printbits(bs, TEST_LENGTH);

  (void) printf("\ntest bit_clear\n");
  for (i=0; i<TEST_LENGTH; i+=6) {
    bit_clear(bs, i); 
    }
  (void) printf("be:   0   3 ");
  for (i=0; i < TEST_LENGTH; i++)
	(void) putchar(*("000100" + (i % 6)));
  (void) printf("\nis: ");
  printbits(bs, TEST_LENGTH);

  (void) printf("\ntest bit_test using previous bitstring\n");
  (void) printf("  i    bit_test(i)\n");
  for (i=0; i<TEST_LENGTH; i++) {
    (void) printf("%3d%15d\n",
      i, bit_test(bs, i));
    }

  clearbits(bs, TEST_LENGTH);
  (void) printf("\ntest clearbits\n");
  (void) printf("be:   0  -1 ");
  for (i=0; i < TEST_LENGTH; i++)
	(void) putchar('0');
  (void) printf("\nis: ");
  printbits(bs, TEST_LENGTH);

  (void) printf("\ntest bit_nset and bit_nclear\n");
  bit_nset(bs, 1, TEST_LENGTH - 2);
  (void) printf("be:   0   1 0");
  for (i=0; i < TEST_LENGTH - 2; i++)
	(void) putchar('1');
  (void) printf("0\nis: ");
  printbits(bs, TEST_LENGTH);

  bit_nclear(bs, 2, TEST_LENGTH - 3);
  (void) printf("be:   0   1 01");
  for (i=0; i < TEST_LENGTH - 4; i++)
        (void) putchar('0');
  (void) printf("10\nis: ");
  printbits(bs, TEST_LENGTH);

  bit_nclear(bs, 0, TEST_LENGTH - 1);
  (void) printf("be:   0  -1 ");
  for (i=0; i < TEST_LENGTH; i++)
	(void) putchar('0');
  (void) printf("\nis: ");
  printbits(bs, TEST_LENGTH);
  bit_nset(bs, 0, TEST_LENGTH - 2);
  (void) printf("be: %3d   0 ",TEST_LENGTH - 1);
  for (i=0; i < TEST_LENGTH - 1; i++)
	(void) putchar('1');
  putchar('0');
  (void) printf("\nis: ");
  printbits(bs, TEST_LENGTH);
  bit_nclear(bs, 0, TEST_LENGTH - 1);
  (void) printf("be:   0  -1 ");
  for (i=0; i < TEST_LENGTH; i++)
	(void) putchar('0');
  (void) printf("\nis: ");
  printbits(bs, TEST_LENGTH);

  (void) printf("\n");
  (void) printf("first 1 bit should move right 1 position each line\n");
  for (i=0; i<TEST_LENGTH; i++) {
    bit_nclear(bs, 0, TEST_LENGTH - 1);
    bit_nset(bs, i, TEST_LENGTH - 1);
    (void) printf("%3d ", i); printbits(bs, TEST_LENGTH);
    }

  (void) printf("\n");
  (void) printf("first 0 bit should move right 1 position each line\n");
  for (i=0; i<TEST_LENGTH; i++) {
    bit_nset(bs, 0, TEST_LENGTH - 1);
    bit_nclear(bs, i, TEST_LENGTH - 1);
    (void) printf("%3d ", i); printbits(bs, TEST_LENGTH);
    }

  (void) printf("\n");
  (void) printf("first 0 bit should move left 1 position each line\n");
  for (i=0; i<TEST_LENGTH; i++) {
    bit_nclear(bs, 0, TEST_LENGTH - 1);
    bit_nset(bs, 0, TEST_LENGTH - 1 - i);
    (void) printf("%3d ", i); printbits(bs, TEST_LENGTH);
    }

  (void) printf("\n");
  (void) printf("first 1 bit should move left 1 position each line\n");
  for (i=0; i<TEST_LENGTH; i++) {
    bit_nset(bs, 0, TEST_LENGTH - 1);
    bit_nclear(bs, 0, TEST_LENGTH - 1 - i);
    (void) printf("%3d ", i); printbits(bs, TEST_LENGTH);
    }

  (void) printf("\n");
  (void) printf("0 bit should move right 1 position each line\n");
  for (i=0; i<TEST_LENGTH; i++) {
    bit_nset(bs, 0, TEST_LENGTH - 1);
    bit_nclear(bs, i, i);
    (void) printf("%3d ", i); printbits(bs, TEST_LENGTH);
    }

  (void) printf("\n");
  (void) printf("1 bit should move right 1 position each line\n");
  for (i=0; i<TEST_LENGTH; i++) {
    bit_nclear(bs, 0, TEST_LENGTH - 1);
    bit_nset(bs, i, i);
    (void) printf("%3d ", i); printbits(bs, TEST_LENGTH);
    }

  (void)free(bs);
  (void)exit(0);
}
void
clearbits(b, n)
  bitstr_t *b;
  int n;
{
  register int i = bitstr_size(n);
  while(i--) 
    *(b + i) = 0;
}
void
printbits(b, n)
  bitstr_t *b;
  int n;
{
  register int i;
  int jc, js;

  bit_ffc(b, n, &jc);
  bit_ffs(b, n, &js);
  (void) printf("%3d %3d ", jc, js);
  for (i=0; i< n; i++) {
    (void) putchar((bit_test(b, i) ? '1' : '0'));
    }
  (void)putchar('\n');
}
