/*	$NetBSD: txsnd.h,v 1.2 2000/01/16 21:47:01 uch Exp $ */

/*
 * Copyright (c) 2000, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

typedef struct tx_sound_tag *tx_sound_tag_t;

struct tx_sound_tag {
	void	*ts_v;
	
	void	(*ts_click)	__P((tx_sound_tag_t));
	void	(*ts_mute)	__P((tx_sound_tag_t, int));
};

#define	tx_sound_click(t) \
	(*((tx_sound_tag_t)(t->tc_soundt))->ts_click) \
	(((tx_sound_tag_t)(t->tc_soundt))->ts_v)
#define	tx_sound_mute(t, onoff) \
	(*((tx_sound_tag_t)(t->tc_soundt))->ts_mute) \
	(((tx_sound_tag_t)(t->tc_soundt))->ts_v, (onoff))

void	tx_sound_init __P((tx_chipset_tag_t));
