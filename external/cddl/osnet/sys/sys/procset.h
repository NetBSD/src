/*	$NetBSD: procset.h,v 1.1.44.1 2018/06/25 07:25:26 pgoyette Exp $	*/

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


#ifndef _SYS_PROCSET_H
#define	_SYS_PROCSET_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/feature_tests.h>
#include <sys/types.h>
#include <sys/signal.h>

/*
 *	This file defines the data needed to specify a set of
 *	processes.  These types are used by the sigsend, sigsendset,
 *	priocntl, priocntlset, waitid, evexit, and evexitset system
 *	calls.
 */
#define	P_INITPID	1
#define	P_INITUID	0
#define	P_INITPGID	0

#include <sys/idtype.h>

/*
 *	The following defines the operations which can be performed to
 *	combine two simple sets of processes to form another set of
 *	processes.
 */
#if !defined(_XPG4_2) || defined(__EXTENSIONS__)
typedef enum idop {
	POP_DIFF,	/* Set difference.  The processes which	*/
			/* are in the left operand set and not	*/
			/* in the right operand set.		*/
	POP_AND,	/* Set disjunction.  The processes	*/
			/* which are in both the left and right	*/
			/* operand sets.			*/
	POP_OR,		/* Set conjunction.  The processes	*/
			/* which are in either the left or the	*/
			/* right operand sets (or both).	*/
	POP_XOR		/* Set exclusive or.  The processes 	*/
			/* which are in either the left or	*/
			/* right operand sets but not in both.	*/
} idop_t;


/*
 *	The following structure is used to define a set of processes.
 *	The set is defined in terms of two simple sets of processes
 *	and an operator which operates on these two operand sets.
 */
typedef struct procset {
	idop_t		p_op;	/* The operator connection the	*/
				/* following two operands each	*/
				/* of which is a simple set of	*/
				/* processes.			*/

	idtype_t	p_lidtype;
				/* The type of the left operand	*/
				/* simple set.			*/
	id_t		p_lid;	/* The id of the left operand.	*/

	idtype_t	p_ridtype;
				/* The type of the right	*/
				/* operand simple set.		*/
	id_t		p_rid;	/* The id of the right operand.	*/
} procset_t;

/*
 *	The following macro can be used to initialize a procset_t
 *	structure.
 */
#define	setprocset(psp, op, ltype, lid, rtype, rid) \
			(psp)->p_op		= (op); \
			(psp)->p_lidtype	= (ltype); \
			(psp)->p_lid		= (lid); \
			(psp)->p_ridtype	= (rtype); \
			(psp)->p_rid		= (rid);

#endif /* !defined(_XPG4_2) || defined(__EXTENSIONS__) */

#ifdef illumos
#ifdef _KERNEL

struct proc;

extern int dotoprocs(procset_t *, int (*)(), char *);
extern int dotolwp(procset_t *, int (*)(), char *);
extern int procinset(struct proc *, procset_t *);
extern int sigsendproc(struct proc *, sigsend_t *);
extern int sigsendset(procset_t *, sigsend_t *);
extern boolean_t cur_inset_only(procset_t *);
extern id_t getmyid(idtype_t);

#endif	/* _KERNEL */
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_PROCSET_H */
