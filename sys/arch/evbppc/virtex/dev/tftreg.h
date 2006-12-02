/* 	$NetBSD: tftreg.h,v 1.1 2006/12/02 22:18:47 freza Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
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

#ifndef _VIRTEX_DEV_LLFBREG_H_
#define _VIRTEX_DEV_LLFBREG_H_

#define TFT_SIZE 		8

/* Used by PLB version, ignored in CDMAC version. */
#define TFT_ADDR 		0x0000 		/* Framebuffer in RAM */
#define ADDR_MASK 		0xffe00000 	/* High 11bits decoded */
#define ADDR_ALIGN 		(~ADDR_MASK + 1)
#define ADDR_MAKE(addr) 	((addr) >> 21) 	/* Write only useful bits */

#define TFT_CTRL 		0x0004
#define CTRL_RESET 		0x80000000 	/* Software reset */
#define CTRL_REVERSE 		0x00000002 	/* Reverse scan direction */
#define CTRL_ENABLE 		0x00000001

#endif /* _VIRTEX_DEV_LLFBREG_H_ */
