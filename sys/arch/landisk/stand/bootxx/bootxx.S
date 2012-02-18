/*	$NetBSD: bootxx.S,v 1.1.104.1 2012/02/18 07:32:27 mrg Exp $	*/

/*-
 * Copyright (C) 2005 NONAKA Kimihiro <nonaka@netbsd.org>
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

#include <machine/asm.h>
#include <sys/bootblock.h>

ENTRY(bootxx_start)
	bra	bootxx_start1
	 nop
	.balign 4
ENTRY(bootxx_magic)
	.long	LANDISK_BOOT_MAGIC_1
boot_params:
	.long	1f - boot_params
#include <boot_params.S>

	. = bootxx_start + 0x80
bootxx_start1:
	mov.l	.L._end, r0		/* zero bss */
	mov.l	.L.__bss_start, r1
	sub	r1, r0
	shlr2	r0			/* _end and __bss_start are aligned */
	mov	#0, r2
1:	mov.l	r2, @r1
	dt	r0
	bf/s	1b
	 add	#4, r1

	mov.l	.L.2nd_loadaddr, r15	/* new stack pointer */
	mov.l	r4, @-r15
	mov.l	.L.boot1, r0
	jsr	@r0
	 mov	r15, r4

	tst	r0, r0
	bf/s	boot_fail
	 mov.l	@r15+, r5		/* pass boot sector to boot */

	/* flush cache */
	mov	#0, r4
	mov	#6, r0
	trapa	#0x3f

	mov.l	.L.2nd_loadaddr, r0
	mov.l	.L.boot_params, r4
	jmp	@r0
	 nop

boot_fail:
	mov	r0, r1
	mova	.L.errtxt, r0
	mov	r0, r4
	mov	#32, r0
	trapa	#0x3f
	mov	r1, r4
	mov	#32, r0
	trapa	#0x3f
	mova	.L.crlf, r0
	mov	r0, r4
	mov	#32, r0
	trapa	#0x3f
99:	bra	99b
	 nop


	.align	2
.L._end:
	.long	_end
.L.__bss_start:
	.long	__bss_start
.L.boot1:
	.long	_C_LABEL(boot1)
.L.2nd_loadaddr:
	.long	SECONDARY_LOAD_ADDRESS
.L.boot_params:
	.long	boot_params

	.align	2
.L.errtxt:	.asciz	"Error: "
	.align	2
.L.crlf:	.asciz	"\r\n"


	/*
	 * Vector the fs calls through here so we can support multiple
	 * file system types with one copy of the library code and
	 * multiple copies of this file.
	 */
ENTRY(xxfs_open)
	mov.l	1f, r0
	jmp	@r0
	 nop
	.align	2
1:	.long	_C_LABEL(XXfs_open)

ENTRY(xxfs_close)
	mov.l	1f, r0
	jmp	@r0
	 nop
	.align	2
1:	.long	_C_LABEL(XXfs_close)

ENTRY(xxfs_read)
	mov.l	1f, r0
	jmp	@r0
	 nop
	.align	2
1:	.long	_C_LABEL(XXfs_read)

ENTRY(xxfs_stat)
	mov.l	1f, r0
	jmp	@r0
	 nop
	.align	2
1:	.long	_C_LABEL(XXfs_stat)



/*
 * int readsects(int dev, uint32_t lba, void *buf, size_t size);
 */
ENTRY(readsects)
	mov	#2, r0
	trapa	#0x3f
	rts
	 nop
argerror:
	rts
	 mov	#-1, r0

/*
 * void putstr(const char *str);
 */
ENTRY(putstr)
	mov	#32, r0
	trapa	#0x3f
	rts
	 nop
