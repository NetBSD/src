/*	$NetBSD: ieee_internal.h,v 1.4.36.1 2002/05/30 15:34:22 gehenna Exp $	*/

/* 
 * IEEE floating point support for NS32081 and NS32381 fpus.
 * Copyright (c) 1995 Ian Dall
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * IAN DALL ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.
 * IAN DALL DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/* 
 *	File:	ieee_internal.h
 *	Author:	Ian Dall
 *	Date:	November 1995
 *
 *	Prototypes and structure definitions internal to the ieee exception
 *      handling package.
 *
 *
 * HISTORY
 * 14-Dec-95  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	First release.
 *
 */
#ifndef _IEEE_INTERNAL_H_
#define _IEEE_INTERNAL_H_
#include "ieee_handler.h"
#include "fpu_status.h"

#ifdef DEBUG
extern int ieee_handler_debug;
#define DP(n, format, args...)  if (ieee_handler_debug >= n) printf(format, ## args)
#else
#define DP(n, format, args...)
#endif

enum format { fmt9, fmt11, fmt12};
#define XOPCODE(fmt, opcode) ((opcode) << 2 | (fmt))
#define MOVF XOPCODE(fmt11, 1)
#define MOVLF XOPCODE(fmt9, 2)
#define MOVFL XOPCODE(fmt9, 3)
#define MOVIF XOPCODE(fmt9, 0)
#define ROUNDFI XOPCODE(fmt9, 4)
#define TRUNCFI XOPCODE(fmt9, 5)
#define FLOORFI XOPCODE(fmt9, 7)
#define ADDF XOPCODE(fmt11, 0)
#define SUBF XOPCODE(fmt11, 4)
#define MULF XOPCODE(fmt11, 12)
#define DIVF XOPCODE(fmt11, 8)
#define NEGF XOPCODE(fmt11, 5)
#define ABSF XOPCODE(fmt11, 13)
#define SCALBF XOPCODE(fmt12, 4)
#define LOGBF XOPCODE(fmt12, 5)
#define DOTF XOPCODE(fmt12, 3)
#define POLYF XOPCODE(fmt12, 2)
#define CMPF XOPCODE(fmt11, 2)
#define LFSR XOPCODE(fmt9, 1)
#define SFSR XOPCODE(fmt9, 6)

union t_conv {
  double d;
  float f;
  struct {
    unsigned int mantissa2:32;
    unsigned int mantissa:20;
    unsigned int exp:11;
    unsigned int sign :1;
  } d_bits;
  struct {
    unsigned int mantissa:23;
    unsigned int exp:8;
    unsigned int sign :1;
  } f_bits;
  signed char c;
  short s;
  int i;
};

/* These assume "double" interpretation of the union is valid */
#define ISNAN(data) ((data).d_bits.exp == 0x7ff && \
		   ((data).d_bits.mantissa != 0 || (data).d_bits.mantissa2 != 0))

#define ISQNAN(data) ((data).d_bits.exp == 0x7ff && \
		    ((data).d_bits.mantissa & 0x80000) != 0)

#define ISSNAN(data) ((data).d_bits.exp == 0x7ff && \
		    ((data).d_bits.mantissa & 0x80000) == 0 && \
		    ((data).d_bits.mantissa != 0 || (data).d_bits.mantissa2 != 0))

#define ISINFTY(data) ((data).d_bits.exp == 0x7ff && (data).d_bits.mantissa == 0 && \
		     (data).d_bits.mantissa2 == 0)

#define ISDENORM(data) ((data).d_bits.exp == 0 && ((data).d_bits.mantissa != 0 || (data).d_bits.mantissa2 != 0))

#define ISZERO(data) ((data).d_bits.exp == 0 && (data).d_bits.mantissa == 0 && (data).d_bits.mantissa2 == 0)

#define EXP_DBIAS 1023

#define EXP_FBIAS 127 

enum op_type {op_type_float, op_type_int};

enum op_class {op_class_read, op_class_write, op_class_rmw, op_class_addr};

enum op_where_tag { op_where_register, op_where_memory, op_where_immediate };

extern const union t_conv infty;

extern const union t_conv snan;

extern const union t_conv qnan;

struct operand {
  enum op_type type;
  enum op_class class;
  struct {
    enum op_where_tag tag;
    vaddr_t addr;
  } where;
  char size;
  union t_conv data;
};


int ieee_undfl(struct operand *op1, struct operand *op2,
	       struct operand *f0_op, int xopcode, state *state);

int ieee_ovfl(struct operand *op1, struct operand *op2,
	      struct operand *f0_op, int xopcode, state *state);

int ieee_dze(struct operand *op1, struct operand *op2,
	     struct operand *f0_op, int xopcode, state *state);

int ieee_invop(struct operand *op1, struct operand *op2,
	       struct operand *f0_op, int xopcode, state *state);

int ieee_add(double data1, double *data2);

int ieee_mul(double data1, double *data2);

int ieee_div(double data1, double *data2);

int ieee_dot(double data1, double data2, double *data3);

void ieee_cmp(double data1, double data2, state *status);

int ieee_normalize(union t_conv *data);

int ieee_scalb(double data1, double *data2);

#endif /* _IEEE_INTERNAL_H_ */
