/*	Id: node.h,v 1.43 2015/11/17 19:19:40 ragge Exp 	*/	
/*	$NetBSD: node.h,v 1.1.1.5 2016/02/09 20:29:16 plunky Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NODE_H
#define NODE_H

/*
 * The attribute struct contains stuff that might be useful in
 * both passes; but currently it's only legal to use it in pass1.
 */
union aarg {
	int iarg;
	char *sarg;
	void *varg;
};

struct attr {
	struct attr *next;
	unsigned int atype:12, sz:2;
	union aarg aa[];
};

#define iarg(x) aa[x].iarg
#define sarg(x) aa[x].sarg
#define varg(x) aa[x].varg

/*
 * The node structure is the basic element in the compiler.
 * Depending on the operator, it may be one of several types.
 *
 * This is rewritten to be a struct instead of a union as it
 * was in the old compiler.
 */
typedef unsigned int TWORD;
#define NIL (NODE *)0

struct regw;

typedef struct node {
	int	n_op;
	union {
		int _reg;
		struct regw *_regw;
	} n_3;
#define	n_reg	n_3._reg
#define	n_regw	n_3._regw
	TWORD	n_type;
	TWORD	n_qual;
	int	n_su;
	union {
		char *	_name;
		int	_label;
#ifdef LANG_CXX
		union	dimfun *_df;
#endif
	} n_5;
	struct attr *n_ap;
	union {
		struct {
			union {
				struct node *_left;
				CONSZ _val;
			} n_l;
			union {
				struct node *_right;
				int _rval;
#ifdef LANG_CXX
				struct symtab *_sp;
#endif
			} n_r;
		} n_u;
		void *_dcon;
#if 0
#ifdef SOFTFLOAT
#ifdef FDFLOAT
		/* To store F- or D-floats */
		struct softfloat {
			unsigned short fd1, fd2, fd3, fd4;
		} _dcon;
#else
#error missing softfloat structure definition
#endif
#else
		long double	_dcon;
#endif
#endif
	} n_f;
} NODE;

#define	n_name	n_5._name
#define	n_df	n_5._df
#define	n_label	n_5._label

#define	n_left	n_f.n_u.n_l._left
#define	n_val	n_f.n_u.n_l._val
#define	n_slval	n_f.n_u.n_l._slval
#define	n_right	n_f.n_u.n_r._right
#define	n_rval	n_f.n_u.n_r._rval
#define	n_sp	n_f.n_u.n_r._sp
#define	n_dcon	n_f._dcon
#define	getlval(p) 	((p)->n_f.n_u.n_l._val)
#define	setlval(p,v)	((p)->n_f.n_u.n_l._val = (v))

#define	NLOCAL1	010000
#define	NLOCAL2	020000
#define	NLOCAL3	040000
/*
 * Node types.
 *
 * MAXOP is the highest number used by the backend.
 */

#define FREE	1
/*
 * Value nodes.
 */
#define NAME	2
#define ICON	4
#define FCON	5
#define REG	6
#define OREG	7
#define TEMP	8
#define XARG	9

/*
 * Arithmetic nodes.
 */
#define PLUS	10
#define MINUS	11
#define DIV	12
#define MOD	13
#define MUL	14

/*
 * Bitwise operations.
 */
#define AND	15
#define OR	16
#define ER	17
#define LS	18
#define RS	19
#define COMPL	20

#define UMUL	23
#define UMINUS	24

/*
 * Logical compare nodes.
 */
#define EQ	25
#define NE	26
#define LE	27
#define LT	28
#define GE	29
#define GT	30
#define ULE	31
#define ULT	32
#define UGE	33
#define UGT	34

/*
 * Branch nodes.
 */
#define CBRANCH	35

/*
 * Convert types.
 */
#define FLD	36
#define SCONV	37
#define PCONV	38
#define PMCONV	39
#define PVCONV	40

/*
 * Function calls.
 */
#define CALL	41
#define	UCALL	42
#define FORTCALL 43
#define UFORTCALL 44
#define STCALL	45
#define USTCALL	46

/*
 *  Other used nodes.
 */
#define CCODES	47
#define CM	48
#define ASSIGN	49
#define STASG	50
#define STARG	51
#define FORCE	52
#define XASM	53
#define	GOTO	54
#define	RETURN	55
#define	FUNARG	57
#define	ADDROF	58

#define	MAXOP	58

#endif
