/*	$NetBSD: swi.h,v 1.1.2.2 2002/02/11 20:07:21 jdolecek Exp $	*/

/*
 * This file is in the Public Domain.
 * Ben Harris, 2002.
 */

#ifndef _ARM_SWI_H_
#define _ARM_SWI_H_

#define SWI_OS_MASK	0xf00000
#define SWI_OS_RISCOS	0x000000
#define SWI_OS_RISCIX	0x800000
#define SWI_OS_LINUX	0x900000
#define SWI_OS_NETBSD	0xa00000
#define SWI_OS_ARM	0xf00000

#define SWI_IMB		0xf00000
#define SWI_IMBrange	0xf00001

#endif

