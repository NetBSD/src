/*-
 * Copyright (c)1999 Citrus Project,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: Id: runeglue.c,v 1.2 2000/02/01 14:58:31 cvscitrus Exp
 */

/*
 * Glue code to hide "rune" facility from user programs.
 * This is important to keep backward/future compatibility.
 */

#define _CTYPE_PRIVATE
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#define _BSD_RUNE_T_    int
#define _BSD_CT_RUNE_T_ _rune_t
#include "runetype.h"

#if EOF != -1
#error "EOF != -1"
#endif
#if _CACHED_RUNES!=256
#error "_CACHED_RUNES!=256"
#endif

static unsigned char __current_ctype[1+_CTYPE_NUM_CHARS];
static short __current_toupper[1+256];
static short __current_tolower[1+256];

void	__runetable_to_netbsd_ctype __P((void));

void
__runetable_to_netbsd_ctype()
{
    int i;
    
    __current_ctype[0] = 0;
    __current_toupper[0] = EOF;
    __current_tolower[0] = EOF;
    for (i=0; i<_CTYPE_NUM_CHARS; i++) {
        __current_ctype[i+1]=0;
        if ( _CurrentRuneLocale->__runetype[i]&___U )
            __current_ctype[i+1]|=_U;
        if ( _CurrentRuneLocale->__runetype[i]&___L )
            __current_ctype[i+1]|=_L;
        if ( _CurrentRuneLocale->__runetype[i]&___D )
            __current_ctype[i+1]|=_N;
        if ( _CurrentRuneLocale->__runetype[i]&___S )
            __current_ctype[i+1]|=_S;
        if ( _CurrentRuneLocale->__runetype[i]&___P )
            __current_ctype[i+1]|=_P;
        if ( _CurrentRuneLocale->__runetype[i]&___C )
            __current_ctype[i+1]|=_C;
        if ( _CurrentRuneLocale->__runetype[i]&___X )
            __current_ctype[i+1]|=_X;
        if ( _CurrentRuneLocale->__runetype[i]&___B )
            __current_ctype[i+1]|=_B;
        __current_toupper[i+1] = (short)_CurrentRuneLocale->__mapupper[i];
        __current_tolower[i+1] = (short)_CurrentRuneLocale->__maplower[i];
    }
}
