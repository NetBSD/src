/*	$NetBSD: syscall_stubs.S,v 1.3 2009/07/20 04:41:37 kiyohara Exp $	*/

	/*-
 * Copyright (c) 2002, 2003 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 *
 * $FreeBSD$
 */

#include <sys/syscall.h>
#include <sys/errno.h>


#include <machine/asm.h>
#include <machine/vmparam.h>
#undef VM_MAX_ADDRESS

#include "assym.h"

/*
 * A process performs a syscall by performing an indirect call to the
 * address stored in ar.k5. The contents of ar.pfs and rp should be
 * saved prior to the syscall in r9 and r10 respectively. The kernel
 * will restore these values on return. The value of gp is preserved
 * across the call. This allows for small enough syscall stubs without
 * getting too weird.
 * The address in ar.k5 is the start of the EPC gateway page and also
 * the syscall entry point. The syscall code in the gateway page is
 * primarily responsible for increasing the privilege level, but will
 * also make sure we have a reliable psr.
 *
 * A process defines:
 *	r8		-	syscall number
 *	r9		-	copy of ar.pfs
 *	r10		-	copy of rp
 *	in0-in7		-	syscall arguments
 *
 * A syscall returns:
 *	r8+r9		-	syscall return value(s)
 *	r10		-	syscall error flag
 *	ar.pfs		-	restored from r9
 *	rp		-	restored from r10
 *	gp		-	preserved
 *
 * The EPC syscall code defines:
 *	r11		-	copy of psr.l
 *	r14		-	Kernel memory stack
 *	r15		-	Kernel register stack
 *
 * Also in the gateway page are the signal trampolines. As such, stacks
 * don't have to be made executable per se. Since debuggers have a need
 * to know about trampolines, we probably need to define a table of
 * vectors or something along those lines so that debuggers can get the
 * information they need and we have the freedom to move code around.
 */

	.section	.text.gateway, "ax"
	.align		PAGE_SIZE
	.global		ia64_gateway_page
ia64_gateway_page:
#warning this is a stub!!!

