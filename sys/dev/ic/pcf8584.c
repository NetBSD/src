/*	$NetBSD: pcf8584.c,v 1.1.4.2 2007/06/09 21:37:17 ad Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tobias Nygren.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Philips PCF8584 I2C Bus Controller
 *
 * This driver does not yet support multi-master arbitration, concurrent access
 * or interrupts, but it should be usable for single-master applications.
 * It is currently used by the envctrl(4) driver on sparc64.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcf8584.c,v 1.1.4.2 2007/06/09 21:37:17 ad Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <machine/bus.h>
#include <machine/param.h>
#include <dev/i2c/i2cvar.h>
#include <dev/ic/pcf8584reg.h>
#include <dev/ic/pcf8584var.h>

static void pcf8584_bus_reset(struct pcf8584_handle *, int);
static int pcf8584_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
			void *, size_t, int);
static int pcf8584_acquire_bus(void *, int);
static void pcf8584_release_bus(void *, int);
static void pcf8584_wait(struct pcf8584_handle *, int);

/*  Must delay for 500 ns between bus accesses according to manual. */
#define DATA_W(x) (DELAY(1), bus_space_write_1(ha->ha_iot, ha->ha_ioh, 0, x))
#define DATA_R() (DELAY(1), bus_space_read_1(ha->ha_iot, ha->ha_ioh, 0))
#define CSR_W(x) (DELAY(1), bus_space_write_1(ha->ha_iot, ha->ha_ioh, 1, x))
#define STATUS_R() (DELAY(1), bus_space_read_1(ha->ha_iot, ha->ha_ioh, 1))
#define BUSY() ((STATUS_R() & PCF8584_STATUS_BBN) == 0)
#define PENDING() ((STATUS_R() & PCF8584_STATUS_PIN) == 0)
#define NAK() ((STATUS_R() & PCF8584_STATUS_LRB) != 0)

/*
 * Wait for an interrupt.
 */
static void
pcf8584_wait(struct pcf8584_handle *ha, int flags)
{
	int timeo;

	if (flags & I2C_F_POLL) {
		timeo = 20;
		while (timeo && !PENDING()) {
			DELAY(1000);
			timeo--;
		}
	} else {
		mutex_enter(&ha->ha_intrmtx);
		cv_timedwait(&ha->ha_intrcond, &ha->ha_intrmtx, mstohz(20));
		mutex_exit(&ha->ha_intrmtx);
	}
}

#ifdef notyet
static void
pcf8584_intr(struct pcf8584_handle *ha) {

	cv_wakeup(&ha->ha_intrcond);
}
#endif

int
pcf8584_init(struct pcf8584_handle *ha)
{

	ha->ha_i2c.ic_cookie = ha;
	ha->ha_i2c.ic_acquire_bus = pcf8584_acquire_bus;
	ha->ha_i2c.ic_release_bus = pcf8584_release_bus;
	ha->ha_i2c.ic_exec = pcf8584_exec;

	mutex_init(&ha->ha_intrmtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&ha->ha_intrcond, "pcf8584");

	pcf8584_bus_reset(ha, I2C_F_POLL);

	return 0;
}

/*
 * Reset i2c bus.
 */
static void
pcf8584_bus_reset(struct pcf8584_handle *ha, int flags)
{

	/* initialize PCF8584 */
	CSR_W(PCF8584_CTRL_PIN);
	DATA_W(0x55);
	CSR_W(PCF8584_CTRL_PIN | PCF8584_REG_S2);
	DATA_W(PCF8584_CLK_12 | PCF8584_SCL_90);
	CSR_W(PCF8584_CTRL_PIN | PCF8584_CTRL_ESO | PCF8584_CTRL_ACK);

	/* XXX needs multi-master synchronization delay here */

	/*
	 * Blindly attempt a write at a nonexistent i2c address (0x7F).
	 * This allows hung i2c devices to pick up the stop condition.
	 */
	DATA_W(0x7F << 1);
	CSR_W(PCF8584_CMD_START);
	pcf8584_wait(ha, flags);
	CSR_W(PCF8584_CMD_STOP);
	pcf8584_wait(ha, flags);
}

static int
pcf8584_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, 
    const void *cmdbuf, size_t cmdlen, void *buf,
    size_t len, int flags)
{
	int i;
	struct pcf8584_handle *ha = cookie;
	uint8_t *p = buf;

	KASSERT(cmdlen == 0);
	KASSERT(op == I2C_OP_READ_WITH_STOP || op == I2C_OP_WRITE_WITH_STOP);

	if (BUSY()) {
		/* We're the only master on the bus, something is wrong. */
		printf("*%s: i2c bus busy!\n", ha->ha_parent->dv_xname);
		pcf8584_bus_reset(ha, flags);
	}
	if (op == I2C_OP_READ_WITH_STOP)
		DATA_W((addr << 1) | 1);
	else
		DATA_W(addr << 1);

	CSR_W(PCF8584_CMD_START);
	pcf8584_wait(ha, flags);
	if (!PENDING()) {
		printf("%s: no intr after i2c sla\n", ha->ha_parent->dv_xname);
	}
	if (NAK())
		goto fail;

	if (op == I2C_OP_READ_WITH_STOP) {
		(void) DATA_R();/* dummy read */
		for (i = 0; i < len; i++) {
			/* wait for a byte to arrive */
			pcf8584_wait(ha, flags);
			if (!PENDING()) {
				printf("%s: lost intr during i2c read\n",
				    ha->ha_parent->dv_xname);
				goto fail;
			}
			if (NAK())
				goto fail;
			if (i == len - 1) {
				/*
				 * we're about to read the final byte, so we
				 * set the controller to NAK the following
				 * byte, if any.
				 */
				CSR_W(PCF8584_CMD_NAK);
			}
			*p++ = DATA_R();
		}
		pcf8584_wait(ha, flags);
		if (!PENDING()) {
			printf("%s: no intr on final i2c nak\n",
			    ha->ha_parent->dv_xname);
			goto fail;
		}
		CSR_W(PCF8584_CMD_STOP);
		(void) DATA_R();/* dummy read */
	} else {
		for (i = 0; i < len; i++) {
			DATA_W(*p++);
			pcf8584_wait(ha, flags);
			if (!PENDING()) {
				printf("%s: no intr during i2c write\n",
				    ha->ha_parent->dv_xname);
				goto fail;
			}
			if (NAK())
				goto fail;
		}
		CSR_W(PCF8584_CMD_STOP);
	}
	pcf8584_wait(ha, flags);
	return 0;
fail:
	CSR_W(PCF8584_CMD_STOP);
	pcf8584_wait(ha, flags);

	return 1;
}

static int
pcf8584_acquire_bus(void *cookie, int flags)
{

	/* XXX concurrent access not yet implemented */
	return 0;
}

static void
pcf8584_release_bus(void *cookie, int flags)
{

}
