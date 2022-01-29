/*	$NetBSD: tpm.c,v 1.25 2022/01/29 12:27:30 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: tpm.c,v 1.25 2022/01/29 12:27:30 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/pmf.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/workqueue.h>

#include <dev/ic/tpmreg.h>
#include <dev/ic/tpmvar.h>

#include "ioconf.h"

CTASSERT(sizeof(struct tpm_header) == 10);

#define TPM_BUFSIZ	1024

#define TPM_PARAM_SIZE	0x0001	/* that's a flag */

/* Timeouts. */
#define TPM_ACCESS_TMO	2000	/* 2sec */
#define TPM_READY_TMO	2000	/* 2sec */
#define TPM_READ_TMO	2000	/* 2sec */
#define TPM_BURST_TMO	2000	/* 2sec */

#define TPM_CAPS_REQUIRED \
	(TPM_INTF_DATA_AVAIL_INT|TPM_INTF_LOCALITY_CHANGE_INT| \
	 TPM_INTF_INT_LEVEL_LOW)

static inline int
tpm_tmotohz(int tmo)
{
	struct timeval tv;

	tv.tv_sec = tmo / 1000;
	tv.tv_usec = 1000 * (tmo % 1000);

	return tvtohz(&tv);
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

		rv = tsleep(sc, PCATCH, "tpm_getburst", 1);
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

static bool
tpm12_suspend(struct tpm_softc *sc)
{
	static const uint8_t command[10] = {
		0x00, 0xC1,		/* TPM_TAG_RQU_COMMAND */
		0x00, 0x00, 0x00, 10,	/* Length in bytes */
		0x00, 0x00, 0x00, 0x98	/* TPM_ORD_SaveState */
	};
	struct tpm_header response;
	size_t nread;
	bool endwrite = false, endread = false;
	int error;

	/*
	 * Write the command.
	 */
	error = (*sc->sc_intf->start)(sc, UIO_WRITE);
	if (error) {
		device_printf(sc->sc_dev, "start write failed: %d", error);
		goto out;
	}

	endwrite = true;

	error = (*sc->sc_intf->write)(sc, &command, sizeof(command));
	if (error) {
		device_printf(sc->sc_dev, "write TPM_ORD_SaveState failed: %d",
		    error);
		goto out;
	}

	endwrite = false;

	error = (*sc->sc_intf->end)(sc, UIO_WRITE, 0);
	if (error) {
		device_printf(sc->sc_dev, "end write failed: %d", error);
		goto out;
	}

	/*
	 * Read the response -- just the header; we don't expect a
	 * payload.
	 */
	error = (*sc->sc_intf->start)(sc, UIO_READ);
	if (error) {
		device_printf(sc->sc_dev, "start read failed: %d", error);
		goto out;
	}

	endread = true;

	error = (*sc->sc_intf->read)(sc, &response, sizeof(response), &nread,
	    0);
	if (error) {
		device_printf(sc->sc_dev, "read failed: %d", error);
		goto out;
	}
	if (nread != sizeof(response)) {
		device_printf(sc->sc_dev, "short header read: %zu", nread);
		goto out;
	}

	endread = false;

	error = (*sc->sc_intf->end)(sc, UIO_READ, 0);
	if (error) {
		device_printf(sc->sc_dev, "end read failed: %d", error);
		goto out;
	}

	/*
	 * Verify the response looks reasonable.
	 */
	if (be16toh(response.tag) != TPM_TAG_RSP_COMMAND ||
	    be32toh(response.length) != sizeof(response) ||
	    be32toh(response.code) != 0) {
		device_printf(sc->sc_dev,
		    "TPM_ORD_SaveState failed: tag=0x%x length=0x%x code=0x%x",
		    be16toh(response.tag),
		    be32toh(response.length),
		    be32toh(response.code));
		error = EIO;
		goto out;
	}

	/* Success!  */
	error = 0;

out:	if (endwrite)
		error = (*sc->sc_intf->end)(sc, UIO_WRITE, error);
	if (endread)
		error = (*sc->sc_intf->end)(sc, UIO_READ, error);
	if (error)
		return false;
	return true;
}

static bool
tpm20_suspend(struct tpm_softc *sc)
{
	static const uint8_t command[12] = {
		0x80, 0x01,		/* TPM_ST_NO_SESSIONS */
		0x00, 0x00, 0x00, 12,	/* Length in bytes */
		0x00, 0x00, 0x01, 0x45,	/* TPM_CC_Shutdown */
		0x00, 0x01		/* TPM_SU_STATE */
	};
	struct tpm_header response;
	size_t nread;
	bool endwrite = false, endread = false;
	int error;

	/*
	 * Write the command.
	 */
	error = (*sc->sc_intf->start)(sc, UIO_WRITE);
	if (error) {
		device_printf(sc->sc_dev, "start write failed: %d", error);
		goto out;
	}

	endwrite = true;

	error = (*sc->sc_intf->write)(sc, &command, sizeof(command));
	if (error) {
		device_printf(sc->sc_dev, "write TPM_ORD_SaveState failed: %d",
		    error);
		goto out;
	}

	endwrite = false;

	error = (*sc->sc_intf->end)(sc, UIO_WRITE, 0);
	if (error) {
		device_printf(sc->sc_dev, "end write failed: %d", error);
		goto out;
	}

	/*
	 * Read the response -- just the header; we don't expect a
	 * payload.
	 */
	error = (*sc->sc_intf->start)(sc, UIO_READ);
	if (error) {
		device_printf(sc->sc_dev, "start read failed: %d", error);
		goto out;
	}

	endread = true;

	error = (*sc->sc_intf->read)(sc, &response, sizeof(response), &nread,
	    0);
	if (error) {
		device_printf(sc->sc_dev, "read failed: %d", error);
		goto out;
	}
	if (nread != sizeof(response)) {
		device_printf(sc->sc_dev, "short header read: %zu", nread);
		goto out;
	}

	endread = false;

	error = (*sc->sc_intf->end)(sc, UIO_READ, 0);
	if (error) {
		device_printf(sc->sc_dev, "end read failed: %d", error);
		goto out;
	}

	/*
	 * Verify the response looks reasonable.
	 */
	if (be16toh(response.tag) != TPM2_ST_NO_SESSIONS ||
	    be32toh(response.length) != sizeof(response) ||
	    be32toh(response.code) != TPM2_RC_SUCCESS) {
		device_printf(sc->sc_dev,
		    "TPM_CC_Shutdown failed: tag=0x%x length=0x%x code=0x%x",
		    be16toh(response.tag),
		    be32toh(response.length),
		    be32toh(response.code));
		error = EIO;
		goto out;
	}

	/* Success!  */
	error = 0;

out:	if (endwrite)
		error = (*sc->sc_intf->end)(sc, UIO_WRITE, error);
	if (endread)
		error = (*sc->sc_intf->end)(sc, UIO_READ, error);
	if (error)
		return false;
	return true;
}

bool
tpm_suspend(device_t dev, const pmf_qual_t *qual)
{
	struct tpm_softc *sc = device_private(dev);

	switch (sc->sc_ver) {
	case TPM_1_2:
		return tpm12_suspend(sc);
	case TPM_2_0:
		return tpm20_suspend(sc);
	default:
		panic("%s: impossible", __func__);
	}
}

bool
tpm_resume(device_t dev, const pmf_qual_t *qual)
{
	/*
	 * Don't do anything, the BIOS is supposed to restore the previously
	 * saved state.
	 */
	return true;
}

/* -------------------------------------------------------------------------- */

static int
tpm_poll(struct tpm_softc *sc, uint8_t mask, int to, wchan_t chan)
{
	int rv;

	while (((sc->sc_status = tpm_status(sc)) & mask) != mask && to--) {
		rv = tsleep(chan, PCATCH, "tpm_poll", 1);
		if (rv && rv != EWOULDBLOCK) {
			return rv;
		}
	}

	return 0;
}

static int
tpm_waitfor(struct tpm_softc *sc, uint8_t bits, int tmo, wchan_t chan)
{
	int retry, to, rv;
	uint8_t todo;

	to = tpm_tmotohz(tmo);
	retry = 3;

restart:
	todo = bits;

	/*
	 * TPM_STS_VALID has priority over the others.
	 */
	if (todo & TPM_STS_VALID) {
		if ((rv = tpm_poll(sc, TPM_STS_VALID, to+1, chan)) != 0)
			return rv;
		todo &= ~TPM_STS_VALID;
	}

	if ((rv = tpm_poll(sc, todo, to, chan)) != 0)
		return rv;

	if ((todo & sc->sc_status) != todo) {
		if ((retry-- > 0) && (bits & TPM_STS_VALID)) {
			bus_space_write_1(sc->sc_bt, sc->sc_bh, TPM_STS,
			    TPM_STS_RESP_RETRY);
			goto restart;
		}
		return EIO;
	}

	return 0;
}

/* -------------------------------------------------------------------------- */

/*
 * TPM using the TIS 1.2 interface.
 */

static int
tpm12_request_locality(struct tpm_softc *sc, int l)
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
		rv = tsleep(sc->sc_intf->init, PCATCH, "tpm_locality", 1);
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
tpm_tis12_probe(bus_space_tag_t bt, bus_space_handle_t bh)
{
	uint32_t cap;
	uint8_t reg;
	int tmo;

	cap = bus_space_read_4(bt, bh, TPM_INTF_CAPABILITY);
	if (cap == 0xffffffff)
		return EINVAL;
	if ((cap & TPM_CAPS_REQUIRED) != TPM_CAPS_REQUIRED)
		return ENOTSUP;

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
		return ETIMEDOUT;
	}

	if (bus_space_read_4(bt, bh, TPM_ID) == 0xffffffff)
		return EINVAL;

	return 0;
}

static int
tpm12_rng(struct tpm_softc *sc, unsigned *entropybitsp)
{
	/*
	 * TPM Specification Version 1.2, Main Part 3: Commands,
	 * Sec. 13.6 TPM_GetRandom
	 */
	struct {
		struct tpm_header hdr;
		uint32_t bytesRequested;
	} __packed command;
	struct response {
		struct tpm_header hdr;
		uint32_t randomBytesSize;
		uint8_t	bytes[64];
	} __packed response;
	bool endwrite = false, endread = false;
	size_t nread;
	uint16_t tag;
	uint32_t pktlen, code, nbytes, entropybits = 0;
	int rv;

	/* Encode the command.  */
	memset(&command, 0, sizeof(command));
	command.hdr.tag = htobe16(TPM_TAG_RQU_COMMAND);
	command.hdr.length = htobe32(sizeof(command));
	command.hdr.code = htobe32(TPM_ORD_GetRandom);
	command.bytesRequested = htobe32(sizeof(response.bytes));

	/* Write the command.   */
	if ((rv = (*sc->sc_intf->start)(sc, UIO_WRITE)) != 0) {
		device_printf(sc->sc_dev, "start write failed, error=%d\n",
		    rv);
		goto out;
	}
	endwrite = true;
	if ((rv = (*sc->sc_intf->write)(sc, &command, sizeof(command))) != 0) {
		device_printf(sc->sc_dev, "write failed, error=%d\n", rv);
		goto out;
	}
	rv = (*sc->sc_intf->end)(sc, UIO_WRITE, 0);
	endwrite = false;
	if (rv) {
		device_printf(sc->sc_dev, "end write failed, error=%d\n", rv);
		goto out;
	}

	/* Read the response header.  */
	if ((rv = (*sc->sc_intf->start)(sc, UIO_READ)) != 0) {
		device_printf(sc->sc_dev, "start write failed, error=%d\n",
		    rv);
		goto out;
	}
	endread = true;
	if ((rv = (*sc->sc_intf->read)(sc, &response.hdr, sizeof(response.hdr),
		    &nread, 0)) != 0) {
		device_printf(sc->sc_dev, "read failed, error=%d\n", rv);
		goto out;
	}

	/* Verify the response header looks sensible.  */
	if (nread != sizeof(response.hdr)) {
		device_printf(sc->sc_dev, "read %zu bytes, expected %zu",
		    nread, sizeof(response.hdr));
		goto out;
	}
	tag = be16toh(response.hdr.tag);
	pktlen = be32toh(response.hdr.length);
	code = be32toh(response.hdr.code);
	if (tag != TPM_TAG_RSP_COMMAND ||
	    pktlen < offsetof(struct response, bytes) ||
	    pktlen > sizeof(response) ||
	    code != 0) {
		/*
		 * If the tpm itself is busy (e.g., it has yet to run a
		 * self-test, or it's in a timeout period to defend
		 * against brute force attacks), then we can try again
		 * later.  Otherwise, give up.
		 */
		if (code & TPM_NON_FATAL) {
			aprint_debug_dev(sc->sc_dev, "%s: tpm busy, code=%u\n",
			    __func__, code & ~TPM_NON_FATAL);
			rv = 0;
		} else if (code == TPM_DEACTIVATED) {
			device_printf(sc->sc_dev, "tpm is deactivated\n");
			rv = ENXIO;
		} else {
			device_printf(sc->sc_dev, "bad tpm response:"
			    " tag=%u len=%u code=%u\n", tag, pktlen, code);
			hexdump(aprint_debug, "tpm response header",
			    (const void *)&response.hdr,
			    sizeof(response.hdr));
			rv = EIO;
		}
		goto out;
	}

	/* Read the response payload.  */
	if ((rv = (*sc->sc_intf->read)(sc,
		    (char *)&response + nread, pktlen - nread,
		    NULL, TPM_PARAM_SIZE)) != 0) {
		device_printf(sc->sc_dev, "read failed, error=%d\n", rv);
		goto out;
	}
	endread = false;
	if ((rv = (*sc->sc_intf->end)(sc, UIO_READ, 0)) != 0) {
		device_printf(sc->sc_dev, "end read failed, error=%d\n", rv);
		goto out;
	}

	/* Verify the number of bytes read looks sensible.  */
	nbytes = be32toh(response.randomBytesSize);
	if (nbytes > pktlen - offsetof(struct response, bytes)) {
		device_printf(sc->sc_dev, "overlong GetRandom length:"
		    " %u, max %zu\n",
		    nbytes, pktlen - offsetof(struct response, bytes));
		nbytes = pktlen - offsetof(struct response, bytes);
	}

	/*
	 * Enter the data into the entropy pool.  Conservatively (or,
	 * perhaps, cargocultily) estimate half a bit of entropy per
	 * bit of data.
	 */
	CTASSERT(sizeof(response.bytes) <= UINT_MAX/(NBBY/2));
	entropybits = (NBBY/2)*nbytes;
	rnd_add_data(&sc->sc_rnd, response.bytes, nbytes, entropybits);

out:	/* End the read or write if still ongoing.  */
	if (endread)
		rv = (*sc->sc_intf->end)(sc, UIO_READ, rv);
	if (endwrite)
		rv = (*sc->sc_intf->end)(sc, UIO_WRITE, rv);

	*entropybitsp = entropybits;
	return rv;
}

static int
tpm20_rng(struct tpm_softc *sc, unsigned *entropybitsp)
{
	/*
	 * Trusted Platform Module Library, Family "2.0", Level 00
	 * Revision 01.38, Part 3: Commands, Sec. 16.1 `TPM2_GetRandom'
	 *
	 * https://trustedcomputinggroup.org/wp-content/uploads/TPM-Rev-2.0-Part-3-Commands-01.38.pdf#page=133
	 */
	struct {
		struct tpm_header hdr;
		uint16_t bytesRequested;
	} __packed command;
	struct response {
		struct tpm_header hdr;
		uint16_t randomBytesSize;
		uint8_t bytes[64];
	} __packed response;
	bool endwrite = false, endread = false;
	size_t nread;
	uint16_t tag;
	uint32_t pktlen, code, nbytes, entropybits = 0;
	int rv;

	/* Encode the command.  */
	memset(&command, 0, sizeof(command));
	command.hdr.tag = htobe16(TPM2_ST_NO_SESSIONS);
	command.hdr.length = htobe32(sizeof(command));
	command.hdr.code = htobe32(TPM2_CC_GetRandom);
	command.bytesRequested = htobe16(sizeof(response.bytes));

	/* Write the command.   */
	if ((rv = (*sc->sc_intf->start)(sc, UIO_WRITE)) != 0) {
		device_printf(sc->sc_dev, "start write failed, error=%d\n",
		    rv);
		goto out;
	}
	endwrite = true;
	if ((rv = (*sc->sc_intf->write)(sc, &command, sizeof(command))) != 0) {
		device_printf(sc->sc_dev, "write failed, error=%d\n", rv);
		goto out;
	}
	rv = (*sc->sc_intf->end)(sc, UIO_WRITE, 0);
	endwrite = false;
	if (rv) {
		device_printf(sc->sc_dev, "end write failed, error=%d\n", rv);
		goto out;
	}

	/* Read the response header.  */
	if ((rv = (*sc->sc_intf->start)(sc, UIO_READ)) != 0) {
		device_printf(sc->sc_dev, "start write failed, error=%d\n",
		    rv);
		goto out;
	}
	endread = true;
	if ((rv = (*sc->sc_intf->read)(sc, &response.hdr, sizeof(response.hdr),
		    &nread, 0)) != 0) {
		device_printf(sc->sc_dev, "read failed, error=%d\n", rv);
		goto out;
	}

	/* Verify the response header looks sensible.  */
	if (nread != sizeof(response.hdr)) {
		device_printf(sc->sc_dev, "read %zu bytes, expected %zu",
		    nread, sizeof(response.hdr));
		goto out;
	}
	tag = be16toh(response.hdr.tag);
	pktlen = be32toh(response.hdr.length);
	code = be32toh(response.hdr.code);
	if (tag != TPM2_ST_NO_SESSIONS ||
	    pktlen < offsetof(struct response, bytes) ||
	    pktlen > sizeof(response) ||
	    code != 0) {
		/*
		 * If the tpm itself is busy (e.g., it has yet to run a
		 * self-test, or it's in a timeout period to defend
		 * against brute force attacks), then we can try again
		 * later.  Otherwise, give up.
		 */
		if (code & TPM2_RC_WARN) {
			aprint_debug_dev(sc->sc_dev, "%s: tpm busy,"
			    " code=TPM_RC_WARN+0x%x\n",
			    __func__, code & ~TPM2_RC_WARN);
			rv = 0;
		} else {
			device_printf(sc->sc_dev, "bad tpm response:"
			    " tag=%u len=%u code=0x%x\n", tag, pktlen, code);
			hexdump(aprint_debug, "tpm response header",
			    (const void *)&response.hdr,
			    sizeof(response.hdr));
			rv = EIO;
		}
		goto out;
	}

	/* Read the response payload.  */
	if ((rv = (*sc->sc_intf->read)(sc,
		    (char *)&response + nread, pktlen - nread,
		    NULL, TPM_PARAM_SIZE)) != 0) {
		device_printf(sc->sc_dev, "read failed, error=%d\n", rv);
		goto out;
	}
	endread = false;
	if ((rv = (*sc->sc_intf->end)(sc, UIO_READ, 0)) != 0) {
		device_printf(sc->sc_dev, "end read failed, error=%d\n", rv);
		goto out;
	}

	/* Verify the number of bytes read looks sensible.  */
	nbytes = be16toh(response.randomBytesSize);
	if (nbytes > pktlen - offsetof(struct response, bytes)) {
		device_printf(sc->sc_dev, "overlong GetRandom length:"
		    " %u, max %zu\n",
		    nbytes, pktlen - offsetof(struct response, bytes));
		nbytes = pktlen - offsetof(struct response, bytes);
	}

	/*
	 * Enter the data into the entropy pool.  Conservatively (or,
	 * perhaps, cargocultily) estimate half a bit of entropy per
	 * bit of data.
	 */
	CTASSERT(sizeof(response.bytes) <= UINT_MAX/(NBBY/2));
	entropybits = (NBBY/2)*nbytes;
	rnd_add_data(&sc->sc_rnd, response.bytes, nbytes, entropybits);

out:	/* End the read or write if still ongoing.  */
	if (endread)
		rv = (*sc->sc_intf->end)(sc, UIO_READ, rv);
	if (endwrite)
		rv = (*sc->sc_intf->end)(sc, UIO_WRITE, rv);

	*entropybitsp = entropybits;
	return rv;
}

static void
tpm_rng_work(struct work *wk, void *cookie)
{
	struct tpm_softc *sc = cookie;
	unsigned nbytes, entropybits;
	int rv;

	/* Acknowledge the request.  */
	nbytes = atomic_swap_uint(&sc->sc_rndpending, 0);

	/* Lock the tpm while we do I/O transactions with it.  */
	mutex_enter(&sc->sc_lock);

	/*
	 * Issue as many commands as needed to fulfill the request, but
	 * stop if anything fails.
	 */
	for (; nbytes; nbytes -= MIN(nbytes, MAX(1, entropybits/NBBY))) {
		switch (sc->sc_ver) {
		case TPM_1_2:
			rv = tpm12_rng(sc, &entropybits);
			break;
		case TPM_2_0:
			rv = tpm20_rng(sc, &entropybits);
			break;
		default:
			panic("bad tpm version: %d", sc->sc_ver);
		}
		if (rv)
			break;
	}

	/*
	 * If the tpm is busted, no sense in trying again -- most
	 * likely, it is deactivated, and by the spec it cannot be
	 * reactivated until after a reboot.
	 */
	if (rv) {
		device_printf(sc->sc_dev, "deactivating entropy source\n");
		atomic_store_relaxed(&sc->sc_rnddisabled, true);
		/* XXX worker thread can't workqueue_destroy its own queue */
	}

	/* Relinquish the tpm.  */
	mutex_exit(&sc->sc_lock);
}

static void
tpm_rng_get(size_t nbytes, void *cookie)
{
	struct tpm_softc *sc = cookie;

	if (atomic_load_relaxed(&sc->sc_rnddisabled))
		return;		/* tough */
	if (atomic_swap_uint(&sc->sc_rndpending, MIN(nbytes, UINT_MAX/NBBY))
	    == 0)
		workqueue_enqueue(sc->sc_rndwq, &sc->sc_rndwk, NULL);
}

static int
tpm_tis12_init(struct tpm_softc *sc)
{
	int rv;

	sc->sc_caps = bus_space_read_4(sc->sc_bt, sc->sc_bh,
	    TPM_INTF_CAPABILITY);
	sc->sc_devid = bus_space_read_4(sc->sc_bt, sc->sc_bh, TPM_ID);
	sc->sc_rev = bus_space_read_1(sc->sc_bt, sc->sc_bh, TPM_REV);

	aprint_normal_dev(sc->sc_dev, "device 0x%08x rev 0x%x\n",
	    sc->sc_devid, sc->sc_rev);

	if ((rv = tpm12_request_locality(sc, 0)) != 0)
		return rv;

	/* Abort whatever it thought it was doing. */
	bus_space_write_1(sc->sc_bt, sc->sc_bh, TPM_STS, TPM_STS_CMD_READY);

	/* XXX Run this at higher priority?  */
	if ((rv = workqueue_create(&sc->sc_rndwq, device_xname(sc->sc_dev),
		    tpm_rng_work, sc, PRI_NONE, IPL_VM, WQ_MPSAFE)) != 0)
		return rv;
	rndsource_setcb(&sc->sc_rnd, tpm_rng_get, sc);
	rnd_attach_source(&sc->sc_rnd, device_xname(sc->sc_dev),
	    RND_TYPE_RNG,
	    RND_FLAG_COLLECT_VALUE|RND_FLAG_ESTIMATE_VALUE|RND_FLAG_HASCB);

	return 0;
}

static int
tpm_tis12_start(struct tpm_softc *sc, int rw)
{
	int rv;

	if (rw == UIO_READ) {
		rv = tpm_waitfor(sc, TPM_STS_DATA_AVAIL | TPM_STS_VALID,
		    TPM_READ_TMO, sc->sc_intf->read);
		return rv;
	}

	/* Request the 0th locality. */
	if ((rv = tpm12_request_locality(sc, 0)) != 0)
		return rv;

	sc->sc_status = tpm_status(sc);
	if (sc->sc_status & TPM_STS_CMD_READY)
		return 0;

	/* Abort previous and restart. */
	bus_space_write_1(sc->sc_bt, sc->sc_bh, TPM_STS, TPM_STS_CMD_READY);
	rv = tpm_waitfor(sc, TPM_STS_CMD_READY, TPM_READY_TMO, sc->sc_intf->write);
	if (rv)
		return rv;

	return 0;
}

static int
tpm_tis12_read(struct tpm_softc *sc, void *buf, size_t len, size_t *count,
    int flags)
{
	uint8_t *p = buf;
	size_t cnt;
	int rv, n;

	cnt = 0;
	while (len > 0) {
		rv = tpm_waitfor(sc, TPM_STS_DATA_AVAIL | TPM_STS_VALID,
		    TPM_READ_TMO, sc->sc_intf->read);
		if (rv)
			return rv;

		n = MIN(len, tpm_getburst(sc));
		while (n > 0) {
			*p++ = bus_space_read_1(sc->sc_bt, sc->sc_bh, TPM_DATA);
			cnt++;
			len--;
			n--;
		}

		if ((flags & TPM_PARAM_SIZE) == 0 && cnt >= 6)
			break;
	}

	if (count)
		*count = cnt;

	return 0;
}

static int
tpm_tis12_write(struct tpm_softc *sc, const void *buf, size_t len)
{
	const uint8_t *p = buf;
	size_t cnt;
	int rv, r;

	if (len == 0)
		return 0;
	if ((rv = tpm12_request_locality(sc, 0)) != 0)
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

static int
tpm_tis12_end(struct tpm_softc *sc, int rw, int err)
{
	int rv = 0;

	if (rw == UIO_READ) {
		rv = tpm_waitfor(sc, TPM_STS_VALID, TPM_READ_TMO, sc->sc_intf->read);
		if (rv)
			goto out;

		/* Still more data? */
		sc->sc_status = tpm_status(sc);
		if (!err && (sc->sc_status & TPM_STS_DATA_AVAIL)) {
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

out:	return err ? err : rv;
}

const struct tpm_intf tpm_intf_tis12 = {
	.version = TIS_1_2,
	.probe = tpm_tis12_probe,
	.init = tpm_tis12_init,
	.start = tpm_tis12_start,
	.read = tpm_tis12_read,
	.write = tpm_tis12_write,
	.end = tpm_tis12_end
};

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
	.d_flag = D_OTHER | D_MPSAFE,
};

static int
tpmopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct tpm_softc *sc = device_lookup_private(&tpm_cd, minor(dev));
	int ret = 0;

	if (sc == NULL)
		return ENXIO;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_busy) {
		ret = EBUSY;
	} else {
		sc->sc_busy = true;
	}
	mutex_exit(&sc->sc_lock);

	return ret;
}

static int
tpmclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct tpm_softc *sc = device_lookup_private(&tpm_cd, minor(dev));
	int ret = 0;

	if (sc == NULL)
		return ENXIO;

	mutex_enter(&sc->sc_lock);
	if (!sc->sc_busy) {
		ret = EINVAL;
	} else {
		sc->sc_busy = false;
	}
	mutex_exit(&sc->sc_lock);

	return ret;
}

static int
tpmread(dev_t dev, struct uio *uio, int flags)
{
	struct tpm_softc *sc = device_lookup_private(&tpm_cd, minor(dev));
	struct tpm_header hdr;
	uint8_t buf[TPM_BUFSIZ];
	size_t cnt, len = 0/*XXXGCC*/;
	bool end = false;
	int rv;

	if (sc == NULL)
		return ENXIO;

	mutex_enter(&sc->sc_lock);

	if ((rv = (*sc->sc_intf->start)(sc, UIO_READ)))
		goto out;
	end = true;

	/* Get the header. */
	if ((rv = (*sc->sc_intf->read)(sc, &hdr, sizeof(hdr), &cnt, 0))) {
		goto out;
	}
	if (cnt != sizeof(hdr)) {
		rv = EIO;
		goto out;
	}
	len = be32toh(hdr.length);
	if (len > MIN(sizeof(buf), uio->uio_resid) || len < sizeof(hdr)) {
		rv = EIO;
		goto out;
	}

	/* Get the payload. */
	len -= sizeof(hdr);
	if ((rv = (*sc->sc_intf->read)(sc, buf, len, NULL, TPM_PARAM_SIZE))) {
		goto out;
	}

out:	if (end)
		rv = (*sc->sc_intf->end)(sc, UIO_READ, rv);

	mutex_exit(&sc->sc_lock);

	/* If anything went wrong, stop here -- nothing to copy out. */
	if (rv)
		return rv;

	/* Copy out the header. */
	if ((rv = uiomove(&hdr, sizeof(hdr), uio))) {
		return rv;
	}

	/* Copy out the payload.  */
	if ((rv = uiomove(buf, len, uio))) {
		return rv;
	}

	/* Success! */
	return 0;
}

static int
tpmwrite(dev_t dev, struct uio *uio, int flags)
{
	struct tpm_softc *sc = device_lookup_private(&tpm_cd, minor(dev));
	uint8_t buf[TPM_BUFSIZ];
	bool end = false;
	int n, rv;

	if (sc == NULL)
		return ENXIO;

	n = MIN(sizeof(buf), uio->uio_resid);
	if ((rv = uiomove(buf, n, uio))) {
		return rv;
	}

	mutex_enter(&sc->sc_lock);

	if ((rv = (*sc->sc_intf->start)(sc, UIO_WRITE))) {
		goto out;
	}
	end = true;

	if ((rv = (*sc->sc_intf->write)(sc, buf, n))) {
		goto out;
	}

out:	if (end)
		rv = (*sc->sc_intf->end)(sc, UIO_WRITE, rv);

	mutex_exit(&sc->sc_lock);
	return rv;
}

static int
tpmioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct tpm_softc *sc = device_lookup_private(&tpm_cd, minor(dev));
	struct tpm_ioc_getinfo *info;

	if (sc == NULL)
		return ENXIO;

	switch (cmd) {
	case TPM_IOC_GETINFO:
		info = addr;
		info->api_version = TPM_API_VERSION;
		info->tpm_version = sc->sc_ver;
		info->itf_version = sc->sc_intf->version;
		info->device_id = sc->sc_devid;
		info->device_rev = sc->sc_rev;
		info->device_caps = sc->sc_caps;
		return 0;
	default:
		break;
	}

	return ENOTTY;
}
