/*	$NetBSD: elf.h,v 1.7 2024/04/01 18:33:23 riastradh Exp $	*/

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 * $FreeBSD: head/sys/cddl/compat/opensolaris/sys/elf.h 177698 2008-03-28 22:16:18Z jb $
 *
 * ELF compatibility definitions for OpenSolaris source.
 *
 */

#ifndef	_SYS__ELF_SOLARIS_H_
#define	_SYS__ELF_SOLARIS_H_

#if HAVE_NBTOOL_CONFIG_H
# include "nbtool_config.h"
#else
# include <sys/types.h>
#endif

/*
 * XXX In the kernel (e.g., zfs module build, or anything else using
 * sys/dtrace.h -> sys/ctf_api.h -> sys/elf.h), sys/elfdefinitions.h
 * isn't available -- it's a header file for the userland libelf.  We
 * could maybe make it available, but sys/exec_elf.h -- which would
 * collide with sys/elfdefinitions.h -- provides all the definitions
 * users need, so we'll just use that for now.
 */
#ifdef _KERNEL
# include <sys/exec_elf.h>
#else
# include <sys/elfdefinitions.h>
#endif

#define	EM_AMD64		EM_X86_64

#define __ELF_WORD_SIZE ELFSIZE

#endif /* !_SYS__ELF_SOLARIS_H_ */
