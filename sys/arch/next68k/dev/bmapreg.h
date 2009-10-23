/*	$NetBSD: bmapreg.h,v 1.3 2009/10/23 02:32:33 snj Exp $	*/

/* 
 * Copyright (c) 2002 Christian Limpach
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


#define BMAP_DDIR (12 * 4)
#define BMAP_DDIR_UTPENABLE_MASK 0x80|0x10

#define BMAP_DATA (13 * 4)
/* observed values:
 * utp: 0xb0 0xd0 0xf0
 * bnc: 0x60
 */
#define BMAP_DATA_UTPCARRIER_MASK 0x20
#define BMAP_DATA_UTPENABLED_MASK 0x10
#define BMAP_DATA_UTPENABLE 0x80|0x10
#define BMAP_DATA_BNCENABLE 0x00

/* Size of register area to be mapped */
#define BMAP_SIZE 14 * 4
