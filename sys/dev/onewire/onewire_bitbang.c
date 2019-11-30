/*	$NetBSD: onewire_bitbang.c,v 1.2 2019/11/30 23:04:12 ad Exp $	*/
/*	$OpenBSD: onewire_bitbang.c,v 1.1 2006/03/04 16:27:03 grange Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2006 Alexander Yurchenko <grange@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: onewire_bitbang.c,v 1.2 2019/11/30 23:04:12 ad Exp $");

/*
 * 1-Wire bus bit-banging routines.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <dev/onewire/onewirevar.h>

 /*
  * Reset the bus.  Sequence from DS18B20 datasheet:
  *
  * 1: Master pulls bus low for a minimum of 480us.
  * 2: Bus pulled up by resistor.  DS18B20 detects rising edge, waits 15-60us.
  * 3: DS18B20 pulls bus low for 60-240us.
  * 4: Bus pulled up by resistor.  Master must wait at least 450us.
  *
  *     111111111112222233333444444444
  *      
  * Vpu |            +-+       +----->
  *     |           /  |      / 
  * GND +----------+   +-----+
  */
int
onewire_bb_reset(const struct onewire_bbops *ops, void *arg)
{
	int s, rv, i;

	rv = 0;
	s = splhigh();
	ops->bb_tx(arg);
	ops->bb_set(arg, 0);
	delay(500);
	ops->bb_set(arg, 1);
	ops->bb_rx(arg);
	for (i = 0; i < 240 / 5; i++) {
		delay(5);
		if ((rv = ops->bb_get(arg)) == 0)
			break;
	}
	splx(s);
	
	/*
	 * After a bus reset, we must wait for at least 450us before any
	 * further device access.  There is no upper bound on this time, so
	 * rather than burning CPU, sleep for 1 tick.
	 */
	KASSERT(2001 > hz);
	(void)kpause("owreset", false, 1, NULL);

	/*
	 * With a push-pull GPIO, bring the bus high to supply
	 * parasite-powered devices.  With either PP or open drain,
	 * past this point on entry to onewire_bb_write_bit() and
	 * onewire_bb_read_bit() we assume that the line is set to
	 * output and not being held low.
	 */
	s = splhigh();
	ops->bb_tx(arg);
	ops->bb_set(arg, 1);
	splx(s);

	return rv;
}

/*
 * Onewire bit write.
 *
 * Method: pull the bus low.  Then let the pull up resistor settle the bus. 
 * ZERO is signalled by the bus being held low for minimum 60us.
 * ONE is signalled by the hold being much shorter (minimum 1us).
 *
 * In any eventuality, pad the entire transaction to the minimum 60us, plus
 * an additional bit of recovery time before the next transaction
 */
void
onewire_bb_write_bit(const struct onewire_bbops *ops, void *arg, int value)
{
	int s, d1, d2;

	if (value) {
		d1 = 2;
		d2 = 62;
	} else {
		d1 = 62;
		d2 = 2;
	}
	
	s = splhigh();
	ops->bb_set(arg, 0);
	delay(d1);
	ops->bb_set(arg, 1);
	splx(s);
	/* Timing no longer critical. */
	delay(d2);
}

/*
 * Onewire bit read.
 *
 * Method: pull the bus low for at least 1us.  Then let the pull up resistor
 * settle the bus.  ZERO is signalled by the bus being pulled low again by
 * the slave at some point between 15-45us of transaction start.  ONE is
 * signalled by the bus not being pulled low.
 *
 * In any eventuality, pad the entire transaction to the minimum 60us, plus
 * an additional bit of recovery time before the next transaction.
 */
int
onewire_bb_read_bit(const struct onewire_bbops *ops, void *arg)
{
	int s, rv, us;

	s = splhigh();
	ops->bb_set(arg, 0);
	delay(2);
	ops->bb_set(arg, 1);
	ops->bb_rx(arg);
	for (us = 62; us >= 60 - 45; us -= 5) {
		delay(5);
		if ((rv = ops->bb_get(arg)) == 0) {
			break;
		}
	}
	splx(s);
	/* Timing no longer critical, and no further need to poll. */
	if (us > 0) {
		delay(us);
	}
	ops->bb_tx(arg);
	ops->bb_set(arg, 1);
	return rv;
}
