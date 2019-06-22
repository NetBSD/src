/*	$NetBSD: tpm.c,v 1.13 2019/06/22 12:57:41 maxv Exp $	*/

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
 * Copyright (c) 2008, 2009 Michael Shalayeff
 * Copyright (c) 2009, 2010 Hans-Joerg Hoexer
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tpm.c,v 1.13 2019/06/22 12:57:41 maxv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/pmf.h>

#include <dev/ic/tpmreg.h>
#include <dev/ic/tpmvar.h>

#include "ioconf.h"

#define TPM_BUFSIZ	1024
#define TPM_HDRSIZE	10
#define TPM_PARAM_SIZE	0x0001	/* that's a flag */

/* Timeouts. */
#define TPM_ACCESS_TMO	2000	/* 2sec */
#define TPM_READY_TMO	2000	/* 2sec */
#define TPM_READ_TMO	2000	/* 2sec */
#define TPM_BURST_TMO	2000	/* 2sec */

#define TPM_CAPS_REQUIRED \
	(TPM_INTF_DATA_AVAIL_INT|TPM_INTF_LOCALITY_CHANGE_INT| \
	 TPM_INTF_INT_LEVEL_LOW)

static const struct {
	uint32_t devid;
	const char *name;
	int flags;
#define TPM_DEV_NOINTS	0x0001
} tpm_devs[] = {
	{ 0x000615d1, "IFX SLD 9630 TT 1.1", 0 },
	{ 0x000b15d1, "IFX SLB 9635 TT 1.2", 0 },
	{ 0x100214e4, "Broadcom BCM0102", TPM_DEV_NOINTS },
	{ 0x00fe1050, "WEC WPCT200", 0 },
	{ 0x687119fa, "SNS SSX35", 0 },
	{ 0x2e4d5453, "STM ST19WP18", 0 },
	{ 0x32021114, "ATML 97SC3203", TPM_DEV_NOINTS },
	{ 0x10408086, "INTEL INTC0102", 0 },
	{ 0, "", TPM_DEV_NOINTS },
};

static inline int
tpm_tmotohz(int tmo)
{
	struct timeval tv;

	tv.tv_sec = tmo / 1000;
	tv.tv_usec = 1000 * (tmo % 1000);

	return tvtohz(&tv);
}

static int
tpm_request_locality(struct tpm_softc *sc, int l)
{
	uint32_t r;
	int to, rv;

	if (l != 0)
		return EINVAL;

	if ((bus_space_read_1(sc->sc_bt, sc->sc_bh, TPM_ACCESS) &
	    (TPM_ACCESS_VALID | TPM_ACCESS_ACTIVE_LOCALITY)) ==
	    (TPM_ACCESS_VALID | TPM_ACCESS_ACTIVE_LOCALITY))
		return 0;

	bus_space_write_1(sc->sc_bt, sc->sc_bh, TPM_ACCESS,
	    TPM_ACCESS_REQUEST_USE);

	to = tpm_tmotohz(TPM_ACCESS_TMO);

	while ((r = bus_space_read_1(sc->sc_bt, sc->sc_bh, TPM_ACCESS) &
	    (TPM_ACCESS_VALID | TPM_ACCESS_ACTIVE_LOCALITY)) !=
	    (TPM_ACCESS_VALID | TPM_ACCESS_ACTIVE_LOCALITY) && to--) {
		rv = tsleep(sc->sc_init, PRIBIO | PCATCH, "tpm_locality", 1);
		if (rv && rv != EWOULDBLOCK) {
			return rv;
		}
	}

	if ((r & (TPM_ACCESS_VALID | TPM_ACCESS_ACTIVE_LOCALITY)) !=
	    (TPM_ACCESS_VALID | TPM_ACCESS_ACTIVE_LOCALITY)) {
		return EBUSY;
	}

	return 0;
}

static int
tpm_getburst(struct tpm_softc *sc)
{
	int burst, to, rv;

	to = tpm_tmotohz(TPM_BURST_TMO);

	while (to--) {
		/*
		 * Burst count is in bits 23:8, so read the two higher bytes.
		 */
		burst = bus_space_read_1(sc->sc_bt, sc->sc_bh, TPM_STS + 1);
		burst |= bus_space_read_1(sc->sc_bt, sc->sc_bh, TPM_STS + 2)
		    << 8;

		if (burst)
			return burst;

		rv = tsleep(sc, PRIBIO | PCATCH, "tpm_getburst", 1);
		if (rv && rv != EWOULDBLOCK) {
			return 0;
		}
	}

	return 0;
}

static inline uint8_t
tpm_status(struct tpm_softc *sc)
{
	return bus_space_read_1(sc->sc_bt, sc->sc_bh, TPM_STS) &
	    TPM_STS_STATUS_BITS;
}

/* -------------------------------------------------------------------------- */

/*
 * Save TPM state on suspend. On resume we don't do anything, since the BIOS
 * is supposed to restore the previously saved state.
 */

bool
tpm12_suspend(device_t dev, const pmf_qual_t *qual)
{
	struct tpm_softc *sc = device_private(dev);
	static const uint8_t command[] = {
		0, 193,		/* TPM_TAG_RQU_COMMAND */
		0, 0, 0, 10,	/* Length in bytes */
		0, 0, 0, 156	/* TPM_ORD_SaveStates */
	};
	uint8_t scratch[sizeof(command)];

	(*sc->sc_write)(sc, &command, sizeof(command));
	(*sc->sc_read)(sc, &scratch, sizeof(scratch), NULL, 0);

	return true;
}

bool
tpm12_resume(device_t dev, const pmf_qual_t *qual)
{
	return true;
}

/* -------------------------------------------------------------------------- */

/*
 * Wait for given status bits using polling.
 */
static int
tpm_waitfor_poll(struct tpm_softc *sc, uint8_t mask, int to, wchan_t chan)
{
	int rv;

	while (((sc->sc_status = tpm_status(sc)) & mask) != mask && to--) {
		rv = tsleep(chan, PRIBIO | PCATCH, "tpm_poll", 1);
		if (rv && rv != EWOULDBLOCK) {
			return rv;
		}
	}

	return 0;
}

/*
 * Wait for given status bits using interrupts.
 */
static int
tpm_waitfor_int(struct tpm_softc *sc, uint8_t mask, int tmo, wchan_t chan,
    int inttype)
{
	int rv, to;

	sc->sc_status = tpm_status(sc);
	if ((sc->sc_status & mask) == mask)
		return 0;

	/*
	 * Enable interrupt on tpm chip.  Note that interrupts on our
	 * level (SPL_TTY) are disabled (see tpm{read,write} et al) and
	 * will not be delivered to the cpu until we call tsleep(9) below.
	 */
	bus_space_write_4(sc->sc_bt, sc->sc_bh, TPM_INT_ENABLE,
	    bus_space_read_4(sc->sc_bt, sc->sc_bh, TPM_INT_ENABLE) |
	    inttype);
	bus_space_write_4(sc->sc_bt, sc->sc_bh, TPM_INT_ENABLE,
	    bus_space_read_4(sc->sc_bt, sc->sc_bh, TPM_INT_ENABLE) |
	    TPM_GLOBAL_INT_ENABLE);

	sc->sc_status = tpm_status(sc);
	if ((sc->sc_status & mask) == mask) {
		rv = 0;
		goto out;
	}

	to = tpm_tmotohz(tmo);

	/*
	 * tsleep(9) enables interrupts on the cpu and returns after
	 * wake up with interrupts disabled again.  Note that interrupts
	 * generated by the tpm chip while being at SPL_TTY are not lost
	 * but held and delivered as soon as the cpu goes below SPL_TTY.
	 */
	rv = tsleep(chan, PRIBIO | PCATCH, "tpm_wait", to);

	sc->sc_status = tpm_status(sc);
	if ((sc->sc_status & mask) == mask)
		rv = 0;

out:
	/* Disable interrupts on tpm chip again. */
	bus_space_write_4(sc->sc_bt, sc->sc_bh, TPM_INT_ENABLE,
	    bus_space_read_4(sc->sc_bt, sc->sc_bh, TPM_INT_ENABLE) &
	    ~TPM_GLOBAL_INT_ENABLE);
	bus_space_write_4(sc->sc_bt, sc->sc_bh, TPM_INT_ENABLE,
	    bus_space_read_4(sc->sc_bt, sc->sc_bh, TPM_INT_ENABLE) &
	    ~inttype);

	return rv;
}

/*
 * Wait on given status bits, use interrupts where possible, otherwise poll.
 */
static int
tpm_waitfor(struct tpm_softc *sc, uint8_t bits, int tmo, wchan_t chan)
{
	int retry, to, rv;
	uint8_t todo;

	/*
	 * We use interrupts for TPM_STS_DATA_AVAIL and TPM_STS_VALID (if the
	 * TPM chip supports them) as waiting for those can take really long.
	 * The other TPM_STS* are not needed very often so we do not support
	 * them.
	 */
	if (sc->sc_vector != -1) {
		todo = bits;

		/*
		 * Wait for data ready. This interrupt only occurs when both
		 * TPM_STS_VALID and TPM_STS_DATA_AVAIL are asserted. Thus we
		 * don't have to bother with TPM_STS_VALID separately and can
		 * just return.
		 *
		 * This only holds for interrupts! When using polling both
		 * flags have to be waited for, see below.
		 */
		if ((bits & TPM_STS_DATA_AVAIL) &&
		    (sc->sc_capabilities & TPM_INTF_DATA_AVAIL_INT))
			return tpm_waitfor_int(sc, bits, tmo, chan,
			    TPM_DATA_AVAIL_INT);

		/* Wait for status valid bit. */
		if ((bits & TPM_STS_VALID) &&
		    (sc->sc_capabilities & TPM_INTF_STS_VALID_INT)) {
			rv = tpm_waitfor_int(sc, bits, tmo, chan,
			    TPM_STS_VALID_INT);
			if (rv)
				return rv;
			todo = bits & ~TPM_STS_VALID;
		}

		/*
		 * When all flags have been taken care of, return. Otherwise
		 * use polling for eg TPM_STS_CMD_READY.
		 */
		if (todo == 0)
			return 0;
	}

	retry = 3;

restart:
	/*
	 * If requested, wait for TPM_STS_VALID before dealing with any other
	 * flag. Eg when both TPM_STS_DATA_AVAIL and TPM_STS_VALID are
	 * requested, wait for the latter first.
	 */
	todo = bits;
	if (bits & TPM_STS_VALID)
		todo = TPM_STS_VALID;
	to = tpm_tmotohz(tmo);
again:
	if ((rv = tpm_waitfor_poll(sc, todo, to, chan)) != 0)
		return rv;

	if ((todo & sc->sc_status) == TPM_STS_VALID) {
		/* Now wait for other flags. */
		todo = bits & ~TPM_STS_VALID;
		to++;
		goto again;
	}

	if ((todo & sc->sc_status) != todo) {
		if (retry-- && (bits & TPM_STS_VALID)) {
			bus_space_write_1(sc->sc_bt, sc->sc_bh, TPM_STS,
			    TPM_STS_RESP_RETRY);
			goto restart;
		}
		return EIO;
	}

	return 0;
}

int
tpm_intr(void *v)
{
	struct tpm_softc *sc = v;
	uint32_t reg;

	reg = bus_space_read_4(sc->sc_bt, sc->sc_bh, TPM_INT_STATUS);
	if (!(reg & (TPM_CMD_READY_INT | TPM_LOCALITY_CHANGE_INT |
	    TPM_STS_VALID_INT | TPM_DATA_AVAIL_INT)))
		return 0;

	if (reg & TPM_STS_VALID_INT)
		wakeup(sc);
	if (reg & TPM_CMD_READY_INT)
		wakeup(sc->sc_write);
	if (reg & TPM_DATA_AVAIL_INT)
		wakeup(sc->sc_read);
	if (reg & TPM_LOCALITY_CHANGE_INT)
		wakeup(sc->sc_init);

	bus_space_write_4(sc->sc_bt, sc->sc_bh, TPM_INT_STATUS, reg);

	return 1;
}

/* -------------------------------------------------------------------------- */

/*
 * TPM using TIS 1.2 interface.
 */

int
tpm_tis12_probe(bus_space_tag_t bt, bus_space_handle_t bh)
{
	uint32_t cap;
	uint8_t reg;
	int tmo;

	cap = bus_space_read_4(bt, bh, TPM_INTF_CAPABILITY);
	if (cap == 0xffffffff)
		return 0;
	if ((cap & TPM_CAPS_REQUIRED) != TPM_CAPS_REQUIRED)
		return 0;
	if (!(cap & (TPM_INTF_INT_EDGE_RISING | TPM_INTF_INT_LEVEL_LOW)))
		return 0;

	/* Request locality 0. */
	bus_space_write_1(bt, bh, TPM_ACCESS, TPM_ACCESS_REQUEST_USE);

	/* Wait for it to become active. */
	tmo = TPM_ACCESS_TMO; /* Milliseconds. */
	while ((reg = bus_space_read_1(bt, bh, TPM_ACCESS) &
	    (TPM_ACCESS_VALID | TPM_ACCESS_ACTIVE_LOCALITY)) !=
	    (TPM_ACCESS_VALID | TPM_ACCESS_ACTIVE_LOCALITY) && tmo--) {
		DELAY(1000); /* 1 millisecond. */
	}
	if ((reg & (TPM_ACCESS_VALID | TPM_ACCESS_ACTIVE_LOCALITY)) !=
	    (TPM_ACCESS_VALID | TPM_ACCESS_ACTIVE_LOCALITY)) {
		return 0;
	}

	if (bus_space_read_4(bt, bh, TPM_ID) == 0xffffffff)
		return 0;

	return 1;
}

static int
tpm_tis12_irqinit(struct tpm_softc *sc, int irq, int idx)
{
	uint32_t reg;

	if ((irq == -1) || (tpm_devs[idx].flags & TPM_DEV_NOINTS)) {
		sc->sc_vector = -1;
		return 0;
	}

	/* Ack and disable all interrupts. */
	reg = bus_space_read_4(sc->sc_bt, sc->sc_bh, TPM_INT_ENABLE);
	bus_space_write_4(sc->sc_bt, sc->sc_bh, TPM_INT_ENABLE,
	    reg & ~TPM_GLOBAL_INT_ENABLE);
	bus_space_write_4(sc->sc_bt, sc->sc_bh, TPM_INT_STATUS,
	    bus_space_read_4(sc->sc_bt, sc->sc_bh, TPM_INT_STATUS));

	/* Program interrupt vector. */
	bus_space_write_1(sc->sc_bt, sc->sc_bh, TPM_INT_VECTOR, irq);
	sc->sc_vector = irq;

	/* Program interrupt type. */
	reg &= ~(TPM_INT_EDGE_RISING|TPM_INT_EDGE_FALLING|TPM_INT_LEVEL_HIGH|
	    TPM_INT_LEVEL_LOW);
	reg |= TPM_GLOBAL_INT_ENABLE|TPM_CMD_READY_INT|TPM_LOCALITY_CHANGE_INT|
	    TPM_STS_VALID_INT|TPM_DATA_AVAIL_INT;
	if (sc->sc_capabilities & TPM_INTF_INT_EDGE_RISING)
		reg |= TPM_INT_EDGE_RISING;
	else if (sc->sc_capabilities & TPM_INTF_INT_EDGE_FALLING)
		reg |= TPM_INT_EDGE_FALLING;
	else if (sc->sc_capabilities & TPM_INTF_INT_LEVEL_HIGH)
		reg |= TPM_INT_LEVEL_HIGH;
	else
		reg |= TPM_INT_LEVEL_LOW;

	bus_space_write_4(sc->sc_bt, sc->sc_bh, TPM_INT_ENABLE, reg);

	return 0;
}

int
tpm_tis12_init(struct tpm_softc *sc, int irq)
{
	int i;

	sc->sc_capabilities = bus_space_read_4(sc->sc_bt, sc->sc_bh,
	    TPM_INTF_CAPABILITY);
	sc->sc_devid = bus_space_read_4(sc->sc_bt, sc->sc_bh, TPM_ID);
	sc->sc_rev = bus_space_read_1(sc->sc_bt, sc->sc_bh, TPM_REV);

	for (i = 0; tpm_devs[i].devid; i++) {
		if (tpm_devs[i].devid == sc->sc_devid)
			break;
	}

	if (tpm_devs[i].devid)
		aprint_normal_dev(sc->sc_dev, "%s rev 0x%x\n",
		    tpm_devs[i].name, sc->sc_rev);
	else
		aprint_normal_dev(sc->sc_dev, "device 0x%08x rev 0x%x\n",
		    sc->sc_devid, sc->sc_rev);

	if (tpm_tis12_irqinit(sc, irq, i))
		return 1;

	if (tpm_request_locality(sc, 0))
		return 1;

	/* Abort whatever it thought it was doing. */
	bus_space_write_1(sc->sc_bt, sc->sc_bh, TPM_STS, TPM_STS_CMD_READY);

	return 0;
}

int
tpm_tis12_start(struct tpm_softc *sc, int flag)
{
	int rv;

	if (flag == UIO_READ) {
		rv = tpm_waitfor(sc, TPM_STS_DATA_AVAIL | TPM_STS_VALID,
		    TPM_READ_TMO, sc->sc_read);
		return rv;
	}

	/* Request the 0th locality. */
	if ((rv = tpm_request_locality(sc, 0)) != 0)
		return rv;

	sc->sc_status = tpm_status(sc);
	if (sc->sc_status & TPM_STS_CMD_READY)
		return 0;

	/* Abort previous and restart. */
	bus_space_write_1(sc->sc_bt, sc->sc_bh, TPM_STS, TPM_STS_CMD_READY);
	rv = tpm_waitfor(sc, TPM_STS_CMD_READY, TPM_READY_TMO, sc->sc_write);
	if (rv)
		return rv;

	return 0;
}

int
tpm_tis12_read(struct tpm_softc *sc, void *buf, size_t len, size_t *count,
    int flags)
{
	uint8_t *p = buf;
	size_t cnt;
	int rv, n, bcnt;

	cnt = 0;
	while (len > 0) {
		rv = tpm_waitfor(sc, TPM_STS_DATA_AVAIL | TPM_STS_VALID,
		    TPM_READ_TMO, sc->sc_read);
		if (rv)
			return rv;

		bcnt = tpm_getburst(sc);
		n = MIN(len, bcnt);

		for (; n--; len--) {
			*p++ = bus_space_read_1(sc->sc_bt, sc->sc_bh, TPM_DATA);
			cnt++;
		}

		if ((flags & TPM_PARAM_SIZE) == 0 && cnt >= 6)
			break;
	}

	if (count)
		*count = cnt;

	return 0;
}

int
tpm_tis12_write(struct tpm_softc *sc, const void *buf, size_t len)
{
	const uint8_t *p = buf;
	size_t cnt;
	int rv, r;

	if (len == 0)
		return 0;
	if ((rv = tpm_request_locality(sc, 0)) != 0)
		return rv;

	cnt = 0;
	while (cnt < len - 1) {
		for (r = tpm_getburst(sc); r > 0 && cnt < len - 1; r--) {
			bus_space_write_1(sc->sc_bt, sc->sc_bh, TPM_DATA, *p++);
			cnt++;
		}
		if ((rv = tpm_waitfor(sc, TPM_STS_VALID, TPM_READ_TMO, sc))) {
			return rv;
		}
		sc->sc_status = tpm_status(sc);
		if (!(sc->sc_status & TPM_STS_DATA_EXPECT)) {
			return EIO;
		}
	}

	bus_space_write_1(sc->sc_bt, sc->sc_bh, TPM_DATA, *p++);
	cnt++;

	if ((rv = tpm_waitfor(sc, TPM_STS_VALID, TPM_READ_TMO, sc))) {
		return rv;
	}
	if ((sc->sc_status & TPM_STS_DATA_EXPECT) != 0) {
		return EIO;
	}

	return 0;
}

int
tpm_tis12_end(struct tpm_softc *sc, int flag, int err)
{
	int rv = 0;

	if (flag == UIO_READ) {
		rv = tpm_waitfor(sc, TPM_STS_VALID, TPM_READ_TMO, sc->sc_read);
		if (rv)
			return rv;

		/* Still more data? */
		sc->sc_status = tpm_status(sc);
		if (!err && ((sc->sc_status & TPM_STS_DATA_AVAIL) ==
		    TPM_STS_DATA_AVAIL)) {
			rv = EIO;
		}

		bus_space_write_1(sc->sc_bt, sc->sc_bh, TPM_STS,
		    TPM_STS_CMD_READY);

		/* Release the 0th locality. */
		bus_space_write_1(sc->sc_bt, sc->sc_bh, TPM_ACCESS,
		    TPM_ACCESS_ACTIVE_LOCALITY);
	} else {
		/* Hungry for more? */
		sc->sc_status = tpm_status(sc);
		if (!err && (sc->sc_status & TPM_STS_DATA_EXPECT)) {
			rv = EIO;
		}

		bus_space_write_1(sc->sc_bt, sc->sc_bh, TPM_STS,
		    err ? TPM_STS_CMD_READY : TPM_STS_GO);
	}

	return rv;
}

/* -------------------------------------------------------------------------- */

static dev_type_open(tpmopen);
static dev_type_close(tpmclose);
static dev_type_read(tpmread);
static dev_type_write(tpmwrite);
static dev_type_ioctl(tpmioctl);

const struct cdevsw tpm_cdevsw = {
	.d_open = tpmopen,
	.d_close = tpmclose,
	.d_read = tpmread,
	.d_write = tpmwrite,
	.d_ioctl = tpmioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER,
};

#define TPMUNIT(a)	minor(a)

static int
tpmopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct tpm_softc *sc = device_lookup_private(&tpm_cd, TPMUNIT(dev));

	if (sc == NULL)
		return ENXIO;
	if (sc->sc_flags & TPM_OPEN)
		return EBUSY;

	sc->sc_flags |= TPM_OPEN;

	return 0;
}

static int
tpmclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct tpm_softc *sc = device_lookup_private(&tpm_cd, TPMUNIT(dev));

	if (sc == NULL)
		return ENXIO;
	if (!(sc->sc_flags & TPM_OPEN))
		return EINVAL;

	sc->sc_flags &= ~TPM_OPEN;

	return 0;
}

static int
tpmread(dev_t dev, struct uio *uio, int flags)
{
	struct tpm_softc *sc = device_lookup_private(&tpm_cd, TPMUNIT(dev));
	uint8_t buf[TPM_BUFSIZ], *p;
	size_t cnt, len, n;
	int  rv, s;

	if (sc == NULL)
		return ENXIO;

	s = spltty();
	if ((rv = (*sc->sc_start)(sc, UIO_READ)))
		goto out;

	if ((rv = (*sc->sc_read)(sc, buf, TPM_HDRSIZE, &cnt, 0))) {
		(*sc->sc_end)(sc, UIO_READ, rv);
		goto out;
	}

	len = (buf[2] << 24) | (buf[3] << 16) | (buf[4] << 8) | buf[5];
	if (len > uio->uio_resid) {
		rv = EIO;
		(*sc->sc_end)(sc, UIO_READ, rv);
		goto out;
	}

	/* Copy out header. */
	if ((rv = uiomove(buf, cnt, uio))) {
		(*sc->sc_end)(sc, UIO_READ, rv);
		goto out;
	}

	/* Get remaining part of the answer (if anything is left). */
	for (len -= cnt, p = buf, n = sizeof(buf); len > 0; p = buf, len -= n,
	    n = sizeof(buf)) {
		n = MIN(n, len);
		if ((rv = (*sc->sc_read)(sc, p, n, NULL, TPM_PARAM_SIZE))) {
			(*sc->sc_end)(sc, UIO_READ, rv);
			goto out;
		}
		p += n;
		if ((rv = uiomove(buf, p - buf, uio))) {
			(*sc->sc_end)(sc, UIO_READ, rv);
			goto out;
		}
	}

	rv = (*sc->sc_end)(sc, UIO_READ, rv);
out:
	splx(s);
	return rv;
}

static int
tpmwrite(dev_t dev, struct uio *uio, int flags)
{
	struct tpm_softc *sc = device_lookup_private(&tpm_cd, TPMUNIT(dev));
	uint8_t buf[TPM_BUFSIZ];
	int n, rv, s;

	if (sc == NULL)
		return ENXIO;

	s = spltty();

	n = MIN(sizeof(buf), uio->uio_resid);
	if ((rv = uiomove(buf, n, uio))) {
		goto out;
	}
	if ((rv = (*sc->sc_start)(sc, UIO_WRITE))) {
		goto out;
	}
	if ((rv = (*sc->sc_write)(sc, buf, n))) {
		goto out;
	}

	rv = (*sc->sc_end)(sc, UIO_WRITE, rv);
out:
	splx(s);
	return rv;
}

static int
tpmioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct tpm_softc *sc = device_lookup_private(&tpm_cd, TPMUNIT(dev));
	struct tpm_ioc_getinfo *info;

	if (sc == NULL)
		return ENXIO;

	switch (cmd) {
	case TPM_IOC_GETINFO:
		info = addr;
		info->api_version = TPM_API_VERSION;
		info->tpm_version = sc->sc_ver;
		info->device_id = sc->sc_devid;
		info->device_rev = sc->sc_rev;
		info->device_caps = sc->sc_capabilities;
		return 0;
	default:
		break;
	}

	return ENOTTY;
}
