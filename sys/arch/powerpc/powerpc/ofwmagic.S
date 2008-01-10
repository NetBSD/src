/*	$NetBSD: ofwmagic.S,v 1.2.140.1 2008/01/10 23:43:57 bouyer Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Magic note section used by OpenFirmware.
 */

	.section ".note","",@note

	# note header

	# length of name
	.long	8

	# note descriptor size
	.long	24

	# note type (IEEE 1275)
	.long	0x1275

	# name of owner
	.asciz	"PowerPC"
	.balign	4


	# note descriptor

	# real mode (-1) or virtual mode (0)
	.long	0

	# real-base
	.long	-1
	# real-size
	.long	-1

	# virt-base
	.long	-1
	# virt-size
	.long	-1
	# load-base 
	.long	0x4000

	# second note is for IBM LPARs
	# length of name
	.long 24
	# note descriptor size
	.long 28
	# note type
	.long 0x12759999
	# name of owner
	.asciz "IBM,RPA-Client-Config"
	.balign 4

	# lpar affinity
	.long 0
	# minimum size in megabytes
	.long 64
	# minimum percentage size
	.long 0
	# max pft size ( 2^48 bytes )
	.long 48
	# splpar
	.long 1
	# min load ?
	.long -1
	# new mem def ?
	.long 0
