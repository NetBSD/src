/*	$NetBSD: bootinfo.h,v 1.1 2001/06/19 00:20:10 fvdl Exp $	*/

#ifndef _X86_64_BOOTINFO_H_
#define _X86_64_BOOTINFO_H_
/*
 * Only the plain i386 info for now, could add more later, but that depends
 * on the eventual architecture of the systems.
 */
#ifdef _KERNEL
#include <i386/include/bootinfo.h>
#else
#include <i386/bootinfo.h>
#endif

#define VAR32_SIZE	4096
#endif	/* _X86_64_BOOTINFO_H_ */
