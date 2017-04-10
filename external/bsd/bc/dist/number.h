/*	$NetBSD: number.h,v 1.1.2.2 2017/04/10 02:28:24 phil Exp $ */
/* number.h: Arbitrary precision numbers header file. */
/*
 * Copyright (C) 1991, 1992, 1993, 1994, 1997, 2012-2017 Free Software Foundation, Inc.
 * Copyright (C) 2000, 2004, 2017 Philip A. Nelson.
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
 * 3. Neither the name of Philip A. Nelson nor the name of the Free Software
 *    Foundation may not be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP A. NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP A. NELSON OR THE FREE SOFTWARE FOUNDATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _NUMBER_H_
#define _NUMBER_H_

typedef enum {PLUS, MINUS} sign;

typedef struct bc_struct *bc_num;

typedef struct bc_struct
    {
      sign  n_sign;
      int   n_len;	/* The number of digits before the decimal point. */
      int   n_scale;	/* The number of digits after the decimal point. */
      int   n_refs;     /* The number of pointers to this number. */
      bc_num n_next;	/* Linked list for available list. */
      char *n_ptr;	/* The pointer to the actual storage.
			   If NULL, n_value points to the inside of
			   another number (bc_multiply...) and should
			   not be "freed." */
      char *n_value;	/* The number. Not zero char terminated.
			   May not point to the same place as n_ptr as
			   in the case of leading zeros generated. */
    } bc_struct;


/* The base used in storing the numbers in n_value above.
   Currently this MUST be 10. */

#define BASE 10

/*  Some useful macros and constants. */

#define CH_VAL(c)     (c - '0')
#define BCD_CHAR(d)   (d + '0')

#ifdef MIN
#undef MIN
#undef MAX
#endif
#define MAX(a,b)      ((a)>(b)?(a):(b))
#define MIN(a,b)      ((a)>(b)?(b):(a))
#define ODD(a)        ((a)&1)

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef LONG_MAX
#define LONG_MAX 0x7fffffff
#endif


/* Global numbers. */
extern bc_num _zero_;
extern bc_num _one_;
extern bc_num _two_;


/* Function Prototypes */

void bc_init_numbers (void);

bc_num bc_new_num (int length, int scale);

void bc_free_num (bc_num *num);

bc_num bc_copy_num (bc_num num);

void bc_init_num (bc_num *num);

void bc_str2num (bc_num *num, char *str, int scale);

char *bc_num2str (bc_num num);

void bc_int2num (bc_num *num, int val);

long bc_num2long (bc_num num);

int bc_compare (bc_num n1, bc_num n2);

char bc_is_zero (bc_num num);

char bc_is_near_zero (bc_num num, int scale);

char bc_is_neg (bc_num num);

void bc_add (bc_num n1, bc_num n2, bc_num *result, int scale_min);

void bc_sub (bc_num n1, bc_num n2, bc_num *result, int scale_min);

void bc_multiply (bc_num n1, bc_num n2, bc_num *prod, int scale);

int bc_divide (bc_num n1, bc_num n2, bc_num *quot, int scale);

int bc_modulo (bc_num num1, bc_num num2, bc_num *result, int scale);

int bc_divmod (bc_num num1, bc_num num2, bc_num *quot,
			   bc_num *rem, int scale);

int bc_raisemod (bc_num base, bc_num expo, bc_num mod,
			     bc_num *result, int scale);

void bc_raise (bc_num num1, bc_num num2, bc_num *result,
			   int scale);

int bc_sqrt (bc_num *num, int scale);

void bc_out_num (bc_num num, int o_base, void (* out_char)(int),
			     int leading_zero);

void bc_out_long (long val, int size, int space, void (*out_char)(int));
#endif
