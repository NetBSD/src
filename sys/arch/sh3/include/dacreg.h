/*	$NetBSD: dacreg.h,v 1.2 2005/12/11 12:18:58 christos Exp $ */

/*
 * Copyright (c) 2003 Valeriy E. Ushakov
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _SH3_DACREG_H_
#define _SH3_DACREG_H_

/* D/A data registers for channels 0 and 1 */
#define SH7709_DADR0	0xa40000a0
#define SH7709_DADR1	0xa40000a2

/* D/A control register */
#define SH7709_DACR	0xa40000a4

#define SH7709_DACR_DAOE1	0x80 /* output enable for channel 1 */
#define SH7709_DACR_DAOE0	0x40 /* output enable for channel 0 */
#define SH7709_DACR_DAE		0x20 /* D/A enable */

#define SH7709_DACR_BITS	\
	"\177\020" "b\07DAOE1\0" "b\06DAOE0\0" "b\05DAE\0"

#endif /* _SH3_DACREG_H_ */
