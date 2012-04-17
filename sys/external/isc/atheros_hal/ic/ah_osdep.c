/*-
 * Copyright (c) 2002-2008 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    similar to the "NO WARRANTY" disclaimer below ("Disclaimer") and any
 *    redistribution must be conditioned upon including a substantially
 *    similar Disclaimer requirement for further binary redistribution.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT, MERCHANTIBILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGES.
 *
 * $Id: ah_osdep.c,v 1.5.2.1 2012/04/17 00:08:17 yamt Exp $
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ah_osdep.c,v 1.5.2.1 2012/04/17 00:08:17 yamt Exp $");

#ifdef _KERNEL_OPT
#include "opt_athhal.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/module.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_arp.h>
#include <net/if_ether.h>

#include <external/isc/atheros_hal/dist/ah.h>

extern	void ath_hal_printf(struct ath_hal *, const char*, ...)
		__printflike(2,3);
extern	void ath_hal_vprintf(struct ath_hal *, const char*, va_list)
		__printflike(2, 0);
extern	const char* ath_hal_ether_sprintf(const u_int8_t *mac);
extern	void *ath_hal_malloc(size_t);
extern	void ath_hal_free(void *);
#ifdef ATHHAL_ASSERT
extern	void ath_hal_assert_failed(const char* filename,
		int lineno, const char* msg);
#endif
#ifdef ATHHAL_DEBUG
extern	void HALDEBUG(struct ath_hal *ah, u_int mask, const char* fmt, ...)
	__printflike(3,4);
#endif /* ATHHAL_DEBUG */

#ifdef ATHHAL_DEBUG
static	int ath_hal_debug = 0;
#endif /* ATHHAL_DEBUG */

int	ath_hal_dma_beacon_response_time = 2;	/* in TU's */
int	ath_hal_sw_beacon_response_time = 10;	/* in TU's */
int	ath_hal_additional_swba_backoff = 0;	/* in TU's */

SYSCTL_SETUP(sysctl_ath_hal, "sysctl ath.hal subtree setup")
{
	int rc;
	const struct sysctlnode *cnode, *rnode;

	if ((rc = sysctl_createv(clog, 0, NULL, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "hw", NULL, NULL, 0, NULL, 0, CTL_HW, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(clog, 0, &rnode, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "ath", SYSCTL_DESCR("Atheros driver parameters"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(clog, 0, &rnode, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "hal", SYSCTL_DESCR("Atheros HAL parameters"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

#if 0
	if ((rc = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_STRING, "version",
	    SYSCTL_DESCR("Atheros HAL version"), NULL, 0, &ath_hal_version, 0,
	    CTL_CREATE, CTL_EOL)) != 0)
		goto err;
#endif

	if ((rc = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "dma_brt",
	    SYSCTL_DESCR("Atheros HAL DMA beacon response time"), NULL, 0,
	    &ath_hal_dma_beacon_response_time, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "sw_brt",
	    SYSCTL_DESCR("Atheros HAL software beacon response time"), NULL, 0,
	    &ath_hal_sw_beacon_response_time, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "swba_backoff",
	    SYSCTL_DESCR("Atheros HAL additional SWBA backoff time"), NULL, 0,
	    &ath_hal_additional_swba_backoff, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

#ifdef ATHHAL_DEBUG
	if ((rc = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Atheros HAL debugging printfs"), NULL, 0,
	    &ath_hal_debug, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;
#endif /* ATHHAL_DEBUG */
	return;
err:
	printf("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}

MALLOC_DEFINE(M_ATH_HAL, "ath_hal", "ath hal data");

void*
ath_hal_malloc(size_t size)
{
	return malloc(size, M_ATH_HAL, M_NOWAIT | M_ZERO);
}

void
ath_hal_free(void* p)
{
	free(p, M_ATH_HAL);
}

void
ath_hal_vprintf(struct ath_hal *ah, const char* fmt, va_list ap)
{
	vprintf(fmt, ap);
}

void
ath_hal_printf(struct ath_hal *ah, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	ath_hal_vprintf(ah, fmt, ap);
	va_end(ap);
}

const char*
ath_hal_ether_sprintf(const u_int8_t *mac)
{
	return ether_sprintf(mac);
}

#ifdef ATHHAL_DEBUG
void
HALDEBUG(struct ath_hal *ah, u_int mask, const char* fmt, ...)
{
	if (ath_hal_debug & mask) {
		va_list ap;
		va_start(ap, fmt);
		ath_hal_vprintf(ah, fmt, ap);
		va_end(ap);
	}
}
#endif /* ATHHAL_DEBUG */

#ifdef ATHHAL_DEBUG_ALQ
/*
 * ALQ register tracing support.
 *
 * Setting hw.ath.hal.alq=1 enables tracing of all register reads and
 * writes to the file /tmp/ath_hal.log.  The file format is a simple
 * fixed-size array of records.  When done logging set hw.ath.hal.alq=0
 * and then decode the file with the arcode program (that is part of the
 * HAL).  If you start+stop tracing the data will be appended to an
 * existing file.
 *
 * NB: doesn't handle multiple devices properly; only one DEVICE record
 *     is emitted and the different devices are not identified.
 */
#include <sys/alq.h>
#include <sys/pcpu.h>

static	struct alq *ath_hal_alq;
static	int ath_hal_alq_emitdev;	/* need to emit DEVICE record */
static	u_int ath_hal_alq_lost;		/* count of lost records */
static	const char *ath_hal_logfile = "/tmp/ath_hal.log";
static	u_int ath_hal_alq_qsize = 64*1024;

static int
ath_hal_setlogging(int enable)
{
	int error;

	if (enable) {
		error = kauth_authorize_network(curlwp->l_cred,
		    KAUTH_NETWORK_INTERFACE,
		    KAUTH_REQ_NETWORK_INTERFACE_SETPRIV, NULL, NULL, NULL);
		if (error == 0) {
			error = alq_open(&ath_hal_alq, ath_hal_logfile,
				curproc->p_ucred,
				sizeof (struct athregrec), ath_hal_alq_qsize);
			ath_hal_alq_lost = 0;
			ath_hal_alq_emitdev = 1;
			printf("ath_hal: logging to %s enabled\n",
				ath_hal_logfile);
		}
	} else {
		if (ath_hal_alq)
			alq_close(ath_hal_alq);
		ath_hal_alq = NULL;
		printf("ath_hal: logging disabled\n");
		error = 0;
	}
	return (error);
}

static int
sysctl_hw_ath_hal_log(SYSCTL_HANDLER_ARGS)
{
	int error, enable;

	enable = (ath_hal_alq != NULL);
        error = sysctl_handle_int(oidp, &enable, 0, req);
        if (error || !req->newptr)
                return (error);
	else
		return (ath_hal_setlogging(enable));
}
SYSCTL_PROC(_hw_ath_hal, OID_AUTO, alq, CTLTYPE_INT|CTLFLAG_RW,
	0, 0, sysctl_hw_ath_hal_log, "I", "Enable HAL register logging");
SYSCTL_INT(_hw_ath_hal, OID_AUTO, alq_size, CTLFLAG_RW,
	&ath_hal_alq_qsize, 0, "In-memory log size (#records)");
SYSCTL_INT(_hw_ath_hal, OID_AUTO, alq_lost, CTLFLAG_RW,
	&ath_hal_alq_lost, 0, "Register operations not logged");

static struct ale *
ath_hal_alq_get(struct ath_hal *ah)
{
	struct ale *ale;

	if (ath_hal_alq_emitdev) {
		ale = alq_get(ath_hal_alq, ALQ_NOWAIT);
		if (ale) {
			struct athregrec *r =
				(struct athregrec *) ale->ae_data;
			r->op = OP_DEVICE;
			r->reg = 0;
			r->val = ah->ah_devid;
			alq_post(ath_hal_alq, ale);
			ath_hal_alq_emitdev = 0;
		} else
			ath_hal_alq_lost++;
	}
	ale = alq_get(ath_hal_alq, ALQ_NOWAIT);
	if (!ale)
		ath_hal_alq_lost++;
	return ale;
}

void
ath_hal_reg_write(struct ath_hal *ah, u_int32_t reg, u_int32_t val)
{
	bus_space_tag_t t = BUSTAG(ah);

	if (ath_hal_alq) {
		struct ale *ale = ath_hal_alq_get(ah);
		if (ale) {
			struct athregrec *r = (struct athregrec *) ale->ae_data;
			r->op = OP_WRITE;
			r->reg = reg;
			r->val = val;
			alq_post(ath_hal_alq, ale);
		}
	}
#if _BYTE_ORDER == _BIG_ENDIAN
	if (reg >= 0x4000 && reg < 0x5000)
		bus_space_write_4(t, h, reg, val);
	else
#endif
		bus_space_write_stream_4(t, h, reg, val);
}

u_int32_t
ath_hal_reg_read(struct ath_hal *ah, u_int32_t reg)
{
	u_int32_t val;
	bus_space_handle_t h = BUSHANDLE(ah);
	bus_space_tag_t t = BUSTAG(ah);

#if _BYTE_ORDER == _BIG_ENDIAN
	if (reg >= 0x4000 && reg < 0x5000)
		val = bus_space_read_4(t, h, reg);
	else
#endif
		val = bus_space_read_stream_4(t, h, reg);

	if (ath_hal_alq) {
		struct ale *ale = ath_hal_alq_get(ah);
		if (ale) {
			struct athregrec *r = (struct athregrec *) ale->ae_data;
			r->op = OP_READ;
			r->reg = reg;
			r->val = val;
			alq_post(ath_hal_alq, ale);
		}
	}

	return val;
}

void
OS_MARK(struct ath_hal *ah, u_int id, u_int32_t v)
{
	if (ath_hal_alq) {
		struct ale *ale = ath_hal_alq_get(ah);

		if (ale) {
			struct athregrec *r = (struct athregrec *) ale->ae_data;
			r->op = OP_MARK;
			r->reg = id;
			r->val = v;
			alq_post(ath_hal_alq, ale);
		}
	}
}
#elif defined(ATHHAL_DEBUG) || defined(AH_REGOPS_FUNC)
/*
 * Memory-mapped device register read/write.  These are here
 * as routines when debugging support is enabled and/or when
 * explicitly configured to use function calls.  The latter is
 * for architectures that might need to do something before
 * referencing memory (e.g. remap an i/o window).
 *
 * NB: see the comments in ah_osdep.h about byte-swapping register
 *     reads and writes to understand what's going on below.
 */

void
ath_hal_reg_write(struct ath_hal *ah, u_int32_t reg, u_int32_t val)
{
	bus_space_handle_t h = BUSHANDLE(ah);
	bus_space_tag_t t = BUSTAG(ah);

#if _BYTE_ORDER == _BIG_ENDIAN
	if (reg >= 0x4000 && reg < 0x5000)
		bus_space_write_4(t, h, reg, val);
	else
#endif
		bus_space_write_stream_4(t, h, reg, val);
}

u_int32_t
ath_hal_reg_read(struct ath_hal *ah, u_int32_t reg)
{
	bus_space_handle_t h = BUSHANDLE(ah);
	bus_space_tag_t t = BUSTAG(ah);
	uint32_t ret;

#if _BYTE_ORDER == _BIG_ENDIAN
	if (reg >= 0x4000 && reg < 0x5000)
		ret = bus_space_read_4(t, h, reg);
	else
#endif
		ret = bus_space_read_stream_4(t, h, reg);

	return ret;
}
#endif /* ATHHAL_DEBUG || AH_REGOPS_FUNC */

#ifdef ATHHAL_ASSERT
void
ath_hal_assert_failed(const char* filename, int lineno, const char *msg)
{
	printf("Atheros HAL assertion failure: %s: line %u: %s\n",
		filename, lineno, msg);
	panic("ath_hal_assert");
}
#endif /* ATHHAL_ASSERT */

/*
 * Delay n microseconds.
 */
void
ath_hal_delay(int n)
{
	DELAY(n);
}

u_int32_t
ath_hal_getuptime(struct ath_hal *ah)
{
	struct bintime bt;
	uint32_t ret;
	getbinuptime(&bt);
	ret = (bt.sec * 1000) +
		(((uint64_t)1000 * (uint32_t)(bt.frac >> 32)) >> 32);
	return ret;
}

void
ath_hal_memzero(void *dst, size_t n)
{
	(void)memset(dst, 0, n);
}

void *
ath_hal_memcpy(void *dst, const void *src, size_t n)
{
	return memcpy(dst, src, n);
}

MODULE(MODULE_CLASS_MISC, ath_hal, NULL);

static int
ath_hal_modcmd(modcmd_t cmd, void *opaque)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
	case MODULE_CMD_FINI:
		return 0;
	default:
		return ENOTTY;
	}
}
