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
 *	$Id: cc.h,v 1.2 1994/02/11 06:59:23 chopps Exp $
 */

#if ! defined (_CC_H)
#define _CC_H

#include <sys/types.h>
#include "cc_types.h"
#include "dlists.h"
#include "custom.h"

#include "cc_vbl.h"
#include "cc_audio.h"
#include "cc_chipmem.h"
#include "cc_copper.h"
#include "cc_blitter.h"

void custom_chips_init (void);
#if ! defined (HIADDR)
#define HIADDR(x) \
        (u_word)((((unsigned long)(x))>>16)&0xffff)
#endif 
#if ! defined (LOADDR)
#define LOADDR(x) \
        (u_word)(((unsigned long)(x))&0xffff)
#endif

#if defined (AMIGA_TEST)
void cc_deinit (void);
int  nothing (void);
void amiga_test_panic (char *,...);
#define splhigh nothing
#define splx(x) 
#define panic amiga_test_panic
#endif

#endif /* _CC_H */

