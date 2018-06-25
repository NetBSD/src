/*	$NetBSD: cyclic.h,v 1.4.28.1 2018/06/25 07:25:25 pgoyette Exp $	*/

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
 *
 * $FreeBSD: head/sys/cddl/compat/opensolaris/sys/cyclic.h 179202 2008-05-22 08:33:24Z jb $
 *
 */
/*
 * Copyright (c) 1999-2001 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef _COMPAT_OPENSOLARIS_SYS_CYCLIC_H_
#define _COMPAT_OPENSOLARIS_SYS_CYCLIC_H_

#ifndef _KERNEL
typedef	void	cpu_t;
#endif


#ifndef _ASM
#include <sys/time.h>
#include <sys/cpuvar.h>
#include <sys/cpupart.h>
#endif /* !_ASM */

#define	CY_LOW_LEVEL		0
#define	CY_LOCK_LEVEL		1
#define	CY_HIGH_LEVEL		2
#define	CY_SOFT_LEVELS		2
#define	CY_LEVELS		3

#ifndef _ASM

typedef uintptr_t cyclic_id_t;
typedef int cyc_index_t;
typedef uint16_t cyc_level_t;
typedef void (*cyc_func_t)(void *);
typedef void *cyb_arg_t;

#define	CYCLIC_NONE		((cyclic_id_t)0)

typedef struct cyc_handler {
	cyc_func_t cyh_func;
	void *cyh_arg;
	cyc_level_t cyh_level;
} cyc_handler_t;

typedef struct cyc_time {
	hrtime_t cyt_when;
	hrtime_t cyt_interval;
} cyc_time_t;

typedef struct cyc_omni_handler {
	void (*cyo_online)(void *, cpu_t *, cyc_handler_t *, cyc_time_t *);
	void (*cyo_offline)(void *, cpu_t *, void *);
	void *cyo_arg;
} cyc_omni_handler_t;

#define	CY_INFINITY	INT64_MAX

#ifdef _KERNEL

cyclic_id_t cyclic_add(cyc_handler_t *, cyc_time_t *);
cyclic_id_t cyclic_add_omni(cyc_omni_handler_t *);
void cyclic_remove(cyclic_id_t);
int cyclic_reprogram(cyclic_id_t, hrtime_t);

#endif /* _KERNEL */

#endif /* !_ASM */

#endif
