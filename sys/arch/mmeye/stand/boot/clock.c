/*	$NetBSD: clock.c,v 1.1 2011/03/03 05:59:37 kiyohara Exp $	*/
/*
 * Copyright (c) 2011 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>

#include <sh3/devreg.h>
#include <sh3/tmureg.h>

#include "boot.h"

void
tmu_init(void)
{

	/* Enable TMU channel 0. */
	_reg_write_1(SH_(TOCR), TOCR_TCOE);
	_reg_write_2(SH_(TCR0), TCR_TPSC_P4);
	_reg_bset_1(SH_(TSTR), TSTR_STR0);
}

void
delay(int n)
{
	int t1, t2, x;

	x = n * 11;
	t1 = _reg_read_4(SH_(TCNT0));
	while (x > 0) {
		t2 = _reg_read_4(SH_(TCNT0));
		if (t2 > t1)
			x -= (t2 - t1 - 1);
		else
			x -= (t1 - t2);
		t1 = t2;
	}
}
