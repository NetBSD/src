/*
 * Copyright (c) 1994 Christian E. Hopps
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
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
 *
 *	$Id: cc_blitter.h,v 1.2 1994/01/29 06:58:40 chopps Exp $
 */

#if ! defined (_CC_BLITTER_H)
#define _CC_BLITTER_H

#include "cc.h"

/* MACROS */
#define BLT_SHIFT_MASK(shift) \
        (0xf&shift)

#define MAKEBOOL(val) \
        (val ? 1 : 0)

#define DMAADDR(lng) \
        (u_long)(((0x7 & lng) << 16)|(lng & 0xffff))

#define MAKE_BLTCON0(shift_a, use_a, use_b, use_c, use_d, minterm) \
        ((0x0000) | (BLT_SHIFT_MASK(shift_a) << 12) | \
	 (use_a << 11) |  (use_b << 10) |  (use_c << 9) |  (use_d << 8) | \
	 (minterm))

#define MAKE_BLTCON1(shift_b, efe, ife, fc, desc)  \
        ((0x0000) | (BLT_SHIFT_MASK(shift_b) << 12) | (efe << 4) | \
	 (ife << 3) | (fc << 2) | (desc << 1))

void cc_init_blitter (void);

#if defined (AMIGA_TEST)
void cc_deinit_blitter (void);
#endif


#endif /* _CC_BLITTER_H */
