/*	$NetBSD: ds28e17iic.c,v 1.1 2025/01/23 19:02:42 brad Exp $	*/

/*
 * Copyright (c) 2025 Brad Spencer <brad@anduin.eldar.org>
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


/* Driver for the DS28E17 1-Wire to I2C bridge chip */

/* https://www.analog.com/en/products/DS28E17.html */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ds28e17iic.c,v 1.1 2025/01/23 19:02:42 brad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysctl.h>

#include <dev/onewire/onewiredevs.h>
#include <dev/onewire/onewirereg.h>
#include <dev/onewire/onewirevar.h>

#include <dev/i2c/i2cvar.h>

#include <dev/onewire/ds28e17iicreg.h>
#include <dev/onewire/ds28e17iicvar.h>

static int	ds28e17iic_match(device_t, cfdata_t, void *);
static void	ds28e17iic_attach(device_t, device_t, void *);
static int	ds28e17iic_detach(device_t, int);
static int	ds28e17iic_activate(device_t, enum devact);
static int 	ds28e17iic_verify_sysctl(SYSCTLFN_ARGS);

#define DS28E17IIC_DEBUG
#ifdef DS28E17IIC_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_ds28e17iicdebug) \
	    printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

CFATTACH_DECL_NEW(ds28e17iic, sizeof(struct ds28e17iic_softc),
	ds28e17iic_match, ds28e17iic_attach, ds28e17iic_detach, ds28e17iic_activate);

extern struct cfdriver ds28e17iic_cd;

static const struct onewire_matchfam ds28e17iic_fams[] = {
	{ ONEWIRE_FAMILY_DS28E17 },
};


#define READY_DELAY(d) if (d > 0) delay(d)

/* The chip uses a 16 bit CRC on the 1-Wire bus when doing any I2C transaction.
 * But it has some strangness to it..  the CRC can be made up from a number of
 * parts of the 1-Wire transaction, some of which are only used for some
 * transactions.  Then the result must be inverted and placed in the proper
 * order.
 */

static uint16_t
ds28e17iic_crc16_bit(uint16_t icrc)
{
	for (size_t i = 0; i < 8; i++) {
		if (icrc & 0x01) {
			icrc >>= 1;
			icrc ^= 0xA001;
		} else {
			icrc >>= 1;
		}
	}

	return(icrc);
}

static void
ds28e17iic_crc16(uint8_t crc16[], uint8_t cmd, uint8_t i2c_addr, uint8_t len, uint8_t *data, uint8_t len2)
{
	uint16_t crc = 0;

	crc ^= cmd;
	crc = ds28e17iic_crc16_bit(crc);

	/* This is a magic value which means that it should not be considered.
	 * The address will never be 0xff, but could, in theory, be 0x00, so
	 * don't use that.
	 */

	if (i2c_addr != 0xff) {
		crc ^= i2c_addr;
		crc = ds28e17iic_crc16_bit(crc);
	}

	crc ^= len;
	crc = ds28e17iic_crc16_bit(crc);

	if (data != NULL) {
		for (size_t j = 0; j < len; j++) {
			crc ^= data[j];
			crc = ds28e17iic_crc16_bit(crc);
		}
	}

	if (len2 > 0) {
		crc ^= len2;
		crc = ds28e17iic_crc16_bit(crc);
	}

	crc = crc ^ 0xffff;

	crc16[1] = crc >> 8;
	crc16[0] = crc & 0xff;
}

int
ds28e17iic_verify_sysctl(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 0)
		return EINVAL;

	*(int *)rnode->sysctl_data = t;

	return 0;
}

/* There isn't much required to acquire or release the I2C bus, but the man
 * pages says these are needed
 */

static int
ds28e17iic_acquire_bus(void *v, int flags)
{
	return(0);
}

static void
ds28e17iic_release_bus(void *v, int flags)
{
	return;
}

/* Perform most of a I2C transaction.  Sometimes there there will be
 * more to read from the 1-Wire bus.  That is device command dependent.
 */

static int
ds28e17iic_ow_i2c_transaction(struct ds28e17iic_softc *sc, const char *what, uint8_t device_cmd,
    uint8_t i2c_addr, uint8_t len1, uint8_t *buf1, uint8_t len2,
    uint8_t crc16[2], uint8_t *i2c_status)
{
	int err = 0;
	int readycount;
	uint8_t ready;

	if (onewire_reset(sc->sc_onewire) != 0) {
		err = EIO;
	} else {
		ds28e17iic_crc16(crc16,device_cmd,i2c_addr,len1,buf1,len2);
		onewire_matchrom(sc->sc_onewire, sc->sc_rom);
		onewire_write_byte(sc->sc_onewire,device_cmd);
		if (i2c_addr != 0xff)
			onewire_write_byte(sc->sc_onewire,i2c_addr);
		onewire_write_byte(sc->sc_onewire,len1);
		if (buf1 != NULL)
			onewire_write_block(sc->sc_onewire,buf1,len1);
		if (len2 > 0)
			onewire_write_byte(sc->sc_onewire,len2);
		onewire_write_block(sc->sc_onewire,crc16,2);
		readycount=0;
		do {
			READY_DELAY(sc->sc_readydelay);
			ready = onewire_read_bit(sc->sc_onewire);
			readycount++;
			if (readycount > sc->sc_readycount)
				err = EAGAIN;
		} while (ready != 0 && !err);
		DPRINTF(sc, 3, ("%s: readycount=%d, err=%d\n",
		    what, readycount, err));
		*i2c_status = onewire_read_byte(sc->sc_onewire);
	}

	return(err);
}

/* This needs to determine what sort of write is going on.  We will may make use
 * of the fact that the chip can start a write transaction with one command and
 * add data to it with a second command.
 */

static int
ds28e17iic_i2c_write(struct ds28e17iic_softc *sc, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *databuf, size_t datalen, int flags)
{
	uint8_t crc16[2];
	uint8_t dcmd;
	uint8_t i2c_status;
	uint8_t i2c_write_status;
	uint8_t *buf;
	size_t len;
	uint8_t maddr;
	int err = 0;

	if (sc->sc_dying)
		return(EIO);

	/* It would be possible to support more than 256 bytes in a transfer by
	 * breaking it up and using the chip's ability to add data to a write,
	 * but that isn't currently done.
	 */

	if (cmdbuf != NULL &&
	    cmdlen > 256)
		return(ENOTSUP);

	if (databuf != NULL &&
	    datalen > 256)
		return(ENOTSUP);

	/* Just null out any attempt to not actually do anything, might save us
	 * a panic */

	if (cmdbuf == NULL &&
	    databuf == NULL)
		return(err);

	maddr = addr << 1;

	onewire_lock(sc->sc_onewire);

	/* Consider two ways to do a basic write.  One is where there is a
	 * cmdbuf and databuf which can use the chip's ability to add to a
	 * ongoing transaction and the other case where there is just the
	 * cmdbuf or the databuf, in which case, just figure out if a stop
	 * is needed.
	 */

	if (cmdbuf != NULL &&
	    databuf != NULL) {
		/* This chip considers a zero length write an error */
		if (cmdlen == 0 ||
		    datalen == 0) {
			if (sc->sc_reportzerolen)
				device_printf(sc->sc_dv,"ds28e17iic_i2c_write: ************ called with zero length read: cmdlen: %zu, datalen: %zu ***************\n",
				    cmdlen, datalen);
		}

		/* In this case, always start out with a write without stop. */

		err = ds28e17iic_ow_i2c_transaction(sc, "ds28e17iic_i2c_write cmd and data 1",
		    DS28E17IIC_DC_WD, maddr, cmdlen, __UNCONST(cmdbuf), 0, crc16, &i2c_status);
		if (! err) {
			i2c_write_status = onewire_read_byte(sc->sc_onewire);
			DPRINTF(sc, 2, ("ds28e17iic_i2c_write: both cmd and data 1: i2c_status=%02x, i2c_write_status=%02x\n",
			    i2c_status, i2c_write_status));
			if (i2c_status == 0 &&
			    i2c_write_status == 0) {
				/* Add data to the transaction and maybe do a stop as well */
				if (I2C_OP_STOP_P(op))
					dcmd = DS28E17IIC_DC_WD_ONLY_WITH_STOP;
				else
					dcmd = DS28E17IIC_DC_WD_ONLY;

				err = ds28e17iic_ow_i2c_transaction(sc, "ds28e17iic_i2c_write cmd and data 2",
				    dcmd, 0xff, datalen, databuf, 0, crc16, &i2c_status);
				if (! err) {
					i2c_write_status = onewire_read_byte(sc->sc_onewire);
					DPRINTF(sc, 2, ("ds28e17iic_i2c_write: both cmd and data 2: dcmd=%02x, i2c_status=%02x, i2c_write_status=%02x\n",
					    dcmd, i2c_status, i2c_write_status));
					if (i2c_status != 0 ||
					    i2c_write_status != 0)
						err = EIO;
				} else {
					DPRINTF(sc, 2, ("ds28e17iic_i2c_write: cmd and data 2: err=%d\n", err));
				}
			} else {
				err = EIO;
			}
		} else {
			DPRINTF(sc, 2, ("ds28e17iic_i2c_write: cmd and data 1: err=%d\n", err));
		}
	} else {
		/* Either the cmdbuf or databuf has something, figure out which
		 * and maybe perform a stop after the write.
		 */
		if (cmdbuf != NULL) {
			buf = __UNCONST(cmdbuf);
			len = cmdlen;
		} else {
			buf = databuf;
			len = datalen;
		}

		if (I2C_OP_STOP_P(op))
			dcmd = DS28E17IIC_DC_WD_WITH_STOP;
		else
			dcmd = DS28E17IIC_DC_WD;

		if (len == 0) {
			if (sc->sc_reportzerolen)
				device_printf(sc->sc_dv,"ds28e17iic_i2c_write: ************ called with zero length read ***************\n");
		}

		err = ds28e17iic_ow_i2c_transaction(sc, "ds28e17iic_i2c_write cmd or data",
		    dcmd, maddr, len, buf, 0, crc16, &i2c_status);
		if (! err) {
			i2c_write_status = onewire_read_byte(sc->sc_onewire);
			DPRINTF(sc, 2, ("ds28e17iic_i2c_write: cmd or data: dcmd=%02x, i2c_status=%02x, i2c_write_status=%02x\n",
			    dcmd, i2c_status, i2c_write_status));
			if (i2c_status != 0 ||
			    i2c_write_status != 0)
				err = EIO;
		} else {
			DPRINTF(sc, 2, ("ds28e17iic_i2c_write: cmd or data: err=%d\n", err));
		}
	}

	onewire_unlock(sc->sc_onewire);

	return(err);
}

/* This deals with the situation where the desire is just to read from
 * the device.  The chip does not support Read without Stop, so turn
 * that into a Read with Stop and hope for the best.
 */

static int
ds28e17iic_i2c_read(struct ds28e17iic_softc *sc, i2c_op_t op, i2c_addr_t addr,
    void *databuf, size_t datalen, int flags)
{
	uint8_t crc16[2];
	uint8_t i2c_status;
	uint8_t maddr;
	int err = 0;

	if (sc->sc_dying)
		return(EIO);

	/* It does not appear that it is possible to read more than 256 bytes */

	if (databuf != NULL &&
	    datalen > 256)
		return(ENOTSUP);

	/* Just null out the attempt to not really read anything */

	if (databuf == NULL)
		return(err);

	maddr = (addr << 1) | 0x01;

	onewire_lock(sc->sc_onewire);

	/* Same thing as a write, a zero length read is considered an error */

	if (datalen == 0)
		if (sc->sc_reportzerolen)
			device_printf(sc->sc_dv,"ds28e17iic_i2c_read: ************ called with zero length read ***************\n");

	if (!I2C_OP_STOP_P(op) &&
	    sc->sc_reportreadnostop)
		device_printf(sc->sc_dv,"ds28e17iic_i2c_read: ************ called with READ without STOP ***************\n");

	err = ds28e17iic_ow_i2c_transaction(sc, "ds28e17iic_i2c_read",
	    DS28E17IIC_DC_RD_WITH_STOP, maddr, datalen,
	    NULL, 0, crc16, &i2c_status);
	if (! err) {
		DPRINTF(sc, 2, ("ds28e17iic_i2c_read: i2c_status=%02x\n", i2c_status));
		if (i2c_status == 0) {
			onewire_read_block(sc->sc_onewire, databuf, datalen);
		} else {
			err = EIO;
		}
	} else {
		DPRINTF(sc, 2, ("ds28e17iic_i2c_read: err=%d\n", err));
	}

	onewire_unlock(sc->sc_onewire);

	return(err);
}

/* This deals with the situation where the desire is to write something to
 * the device and then read some stuff back.  The chip does not support Read
 * without Stop, so turn that into a Read with Stop and hope for the best.
 */

static int
ds28e17iic_i2c_write_read(struct ds28e17iic_softc *sc, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *databuf, size_t datalen, int flags)
{
	uint8_t crc16[2];
	uint8_t i2c_status;
	uint8_t i2c_write_status;
	uint8_t maddr;
	int err = 0;

	if (sc->sc_dying)
		return(EIO);

	/* When using the write+read command of the chip, it does not appear to be
	 * possible to read more than 256 bytes, or write more than 256 bytes in a
	 * transaction.
	 */

	if (cmdbuf != NULL &&
	    cmdlen > 256)
		return(ENOTSUP);

	if (databuf != NULL &&
	    datalen > 256)
		return(ENOTSUP);

	/* Just return if asked to not actually do anything */

	if (cmdbuf == NULL &&
	    databuf == NULL)
		return(err);

	maddr = addr << 1;

	/* Same as a single read or write, a zero length anything here is
	 * considered an error.
	 */

	if (cmdlen == 0 ||
	    datalen == 0) {
		if (sc->sc_reportzerolen)
			device_printf(sc->sc_dv,"ds28e17iic_i2c_write_read: ************ called with zero length read: cmdlen: %zu, datalen: %zu ***************\n",
			    cmdlen, datalen);
	}

	if (!I2C_OP_STOP_P(op) &&
	    sc->sc_reportreadnostop)
		device_printf(sc->sc_dv,"ds28e17iic_i2c_write_read: ************ called with READ without STOP ***************\n");

	onewire_lock(sc->sc_onewire);

	err = ds28e17iic_ow_i2c_transaction(sc, "ds28e17iic_i2c_write_read",
	    DS28E17IIC_DC_WD_RD_WITH_STOP, maddr, cmdlen,
	    __UNCONST(cmdbuf), datalen, crc16, &i2c_status);
	if (! err) {
		/* Like with the normal write, even if the i2c_status is not zero,
		 * we have to read the write status too.  It will end up being
		 * the value 0xff. */
		i2c_write_status = onewire_read_byte(sc->sc_onewire);
		DPRINTF(sc, 2, ("ds28e17iic_i2c_write_read: i2c_status=%02x, i2c_write_status=%02x\n",
		    i2c_status, i2c_write_status));
		/* However, don't bother trying to read the data block if there was an error */
		if (i2c_status == 0 &&
		    i2c_write_status == 0) {
			onewire_read_block(sc->sc_onewire, databuf, datalen);
		} else {
			err = EIO;
		}
	} else {
		DPRINTF(sc, 2, ("ds28e17iic_i2c_write_read: err=%d\n", err));
	}

	onewire_unlock(sc->sc_onewire);

	return(err);
}

/* This needs to figure out what sort of thing is being sent to the end device.
 * The chip will help out with some of this.
 */

static int
ds28e17iic_i2c_exec(void *v, i2c_op_t op, i2c_addr_t addr, const void *cmdbuf,
    size_t cmdlen, void *databuf, size_t datalen, int flags)
{
	struct ds28e17iic_softc *sc = v;
	int err = 0;

	if (sc->sc_dying)
		return(EIO);

	/* The chip only supports 7 bit addressing */

	if (addr > 0x7f)
		return (ENOTSUP);

	/* XXX - this driver could support setting the speed for this I2C
	 * transaction as that information is in the flags, but nothing really
	 * asks to do that, so just ignore that for now.
	 *
	 * If it did support setting speed, do it here.
	 *
	 */

	if (I2C_OP_WRITE_P(op)) {
		/* A write may include the cmdbuf and/or the databuf */
		err = ds28e17iic_i2c_write(sc, op, addr, cmdbuf, cmdlen, databuf, datalen, flags);

		DPRINTF(sc, 2, ("ds28e17iic_exec: I2C WRITE: addr=%02x, err=%d\n", addr, err));
	} else {
		/* If a read includes a cmdbuf, then this is a write+read operation */
		if (cmdbuf != NULL)
			err = ds28e17iic_i2c_write_read(sc, op, addr, cmdbuf, cmdlen, databuf, datalen, flags);
		else
			err = ds28e17iic_i2c_read(sc, op, addr, databuf, datalen, flags);

		DPRINTF(sc, 2, ("ds28e17iic_exec: I2C %s addr=%02x, err=%d\n",
		    cmdbuf != NULL ? "WRITE-READ" : "READ", addr, err));
	}

	return(err);
}

static int
ds28e17iic_sysctl_init(struct ds28e17iic_softc *sc)
{
	int error;
	const struct sysctlnode *cnode;
	int sysctlroot_num;

	if ((error = sysctl_createv(&sc->sc_ds28e17iiclog, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dv),
	    SYSCTL_DESCR("DS28E17IIC controls"), NULL, 0, NULL, 0, CTL_HW,
	    CTL_CREATE, CTL_EOL)) != 0)
		return error;

	sysctlroot_num = cnode->sysctl_num;

#ifdef DS28E17IIC_DEBUG
	if ((error = sysctl_createv(&sc->sc_ds28e17iiclog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Debug level"), ds28e17iic_verify_sysctl, 0,
	    &sc->sc_ds28e17iicdebug, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;
#endif

	if ((error = sysctl_createv(&sc->sc_ds28e17iiclog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "readycount",
	    SYSCTL_DESCR("How many times to wait for the I2C transaction to finish"), ds28e17iic_verify_sysctl, 0,
	    &sc->sc_readycount, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_ds28e17iiclog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "readydelay",
	    SYSCTL_DESCR("Delay in microseconds before checking the I2C transaction"), ds28e17iic_verify_sysctl, 0,
	    &sc->sc_readydelay, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_ds28e17iiclog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "reportreadnostop",
	    SYSCTL_DESCR("Report that a READ without STOP was attempted by a device"),
	    NULL, 0, &sc->sc_reportreadnostop, 0, CTL_HW, sysctlroot_num,
	    CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_ds28e17iiclog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "reportzerolen",
	    SYSCTL_DESCR("Report that an attempt to read or write zero length was attempted"),
	    NULL, 0, &sc->sc_reportzerolen, 0, CTL_HW, sysctlroot_num,
	    CTL_CREATE, CTL_EOL)) != 0)
		return error;

	return 0;
}

static int
ds28e17iic_match(device_t parent, cfdata_t match, void *aux)
{
	return (onewire_matchbyfam(aux, ds28e17iic_fams,
	    __arraycount(ds28e17iic_fams)));
}

static void
ds28e17iic_attach(device_t parent, device_t self, void *aux)
{
	struct ds28e17iic_softc *sc = device_private(self);
	struct onewire_attach_args *oa = aux;
	struct i2cbus_attach_args iba;
	uint8_t hardware_rev;
	uint8_t i2c_speed[2];
	int err = 0;

	aprint_normal("\n");

	sc->sc_dv = self;
	sc->sc_onewire = oa->oa_onewire;
	sc->sc_rom = oa->oa_rom;
	sc->sc_reportreadnostop = true;
	sc->sc_reportzerolen = true;
	sc->sc_ds28e17iicdebug = 0;
	sc->sc_readycount=20;
	sc->sc_readydelay = 10;

	if ((err = ds28e17iic_sysctl_init(sc)) != 0) {
		aprint_error_dev(self, "Can't setup sysctl tree (%d)\n", err);
		return;
	}

	onewire_lock(sc->sc_onewire);

	if (onewire_reset(sc->sc_onewire) != 0) {
		aprint_error_dev(sc->sc_dv, "Could not reset the onewire bus for hardware version\n");
		onewire_unlock(sc->sc_onewire);
		return;
	}
	onewire_matchrom(sc->sc_onewire, sc->sc_rom);
	onewire_write_byte(sc->sc_onewire, DS28E17IIC_DC_DEV_REVISION);
	hardware_rev = onewire_read_byte(sc->sc_onewire);

	aprint_normal_dev(sc->sc_dv, "Hardware revision: %d\n",
	    hardware_rev);

	/* The default for the chip is 400Khz, but some devices may not be able
	 * to deal with that, so set the speed to 100Khz which is standard I2C
	 * speed.
	 */

	if (onewire_reset(sc->sc_onewire) != 0) {
		aprint_error_dev(sc->sc_dv, "Could not reset the onewire bus to set I2C speed\n");
		onewire_unlock(sc->sc_onewire);
		return;
	}
	onewire_matchrom(sc->sc_onewire, sc->sc_rom);
	i2c_speed[0] = DS28E17IIC_DC_WRITE_CONFIG;
	i2c_speed[1] = DS28E17IIC_SPEED_100KHZ;
	onewire_write_block(sc->sc_onewire, i2c_speed, 2);

	onewire_unlock(sc->sc_onewire);

	iic_tag_init(&sc->sc_i2c_tag);
	sc->sc_i2c_tag.ic_cookie = sc;
	sc->sc_i2c_tag.ic_acquire_bus = ds28e17iic_acquire_bus;
	sc->sc_i2c_tag.ic_release_bus = ds28e17iic_release_bus;
	sc->sc_i2c_tag.ic_exec = ds28e17iic_i2c_exec;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c_tag;
	sc->sc_i2c_dev = config_found(self, &iba, iicbus_print, CFARGS(.iattr = "i2cbus"));
}

static int
ds28e17iic_detach(device_t self, int flags)
{
	struct ds28e17iic_softc *sc = device_private(self);
	int err = 0;

	sc->sc_dying = 1;

	err = config_detach_children(self, flags);
	if (err)
		return err;

	sysctl_teardown(&sc->sc_ds28e17iiclog);

	return 0;
}

static int
ds28e17iic_activate(device_t self, enum devact act)
{
	struct ds28e17iic_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}


MODULE(MODULE_CLASS_DRIVER, ds28e17iic, "onewire,iic");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
ds28e17iic_modcmd(modcmd_t cmd, void *opaque)
{
	int error;

	error = 0;
	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_ds28e17iic,
		    cfattach_ioconf_ds28e17iic, cfdata_ioconf_ds28e17iic);
		if (error)
			aprint_error("%s: unable to init component\n",
			    ds28e17iic_cd.cd_name);
#endif
		break;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		config_fini_component(cfdriver_ioconf_ds28e17iic,
		    cfattach_ioconf_ds28e17iic, cfdata_ioconf_ds28e17iic);
#endif
		break;
	default:
		error = ENOTTY;
	}
	return error;
}
