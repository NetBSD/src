/*	$NetBSD: leds.c,v 1.1.12.2 2002/03/07 14:44:04 simonb Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>

#include <mips/cpuregs.h>

#include <evbmips/malta/maltareg.h>

void
led_bar(uint8_t val)
{
	uint8_t *ledbar = (uint8_t *)MIPS_PHYS_TO_KSEG1(MALTA_LEDBAR);

	*ledbar = val;
}

void
led_display_word(uint32_t val)
{
	uint32_t *ledbar = (uint32_t *)MIPS_PHYS_TO_KSEG1(MALTA_ASCIIWORD);

	*ledbar = val;
}

void
led_display_str(const char *str)
{
	uint8_t *leds = (uint8_t *)MIPS_PHYS_TO_KSEG1(MALTA_ASCII_BASE);
	int i;

	for (i = 0; *str != 0 && i <= MALTA_ASCIIPOS7 + 1; str++, i += 8)
		leds[i] = *str;
	/* Fill with spaces */
	for (; i <= MALTA_ASCIIPOS7; i += 8)
		leds[i] = ' ';
}

void
led_display_char(int pos, uint8_t val)
{
	uint8_t *leds = (uint8_t *)MIPS_PHYS_TO_KSEG1(MALTA_ASCII_BASE);

	if (pos > 7)
		return;

	leds[pos * 8] = val;
}
