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
 *	$Id: cc_audio.h,v 1.2 1994/01/29 06:58:38 chopps Exp $
 */

#if ! defined (_CC_AUDIO_H)
#define _CC_AUDIO_H

#include "cc.h"

void cc_init_audio (void);

/*--------------------------------------------------
 * play_sample (len, data, period, volume,
 *              channels, count);
 * play_sample (u_word, u_word *, u_word, u_byte,
 *              u_byte, u_long);
 *
 * play the ``len'' of samples pointed to by ``data''
 * with ``period'' at ``volume''  on ``channels''
 * for ``vbl_count'' number of vertical blanks.
 *
 * len  - number of uwords that make up the data.
 * data - the actual words of audio data.
 * period - the period between sample fetch.
 * volume - the decbel level to play the sound at.
 * channels  - bits 0-3 set for which channels to use.
 * count - the number of time the sample is played.
 *--------------------------------------------------*/
void play_sample (u_word len, u_word *data, u_word period, u_word volume, u_word channels, u_long count);


#endif /* _CC_AUDIO_H */
