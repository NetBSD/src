/* $NetBSD: viapcib.c,v 1.6.42.1 2008/01/08 22:10:07 bouyer Exp $ */
/* $FreeBSD: src/sys/pci/viapm.c,v 1.10 2005/05/29 04:42:29 nyan Exp $ */

/*-
 * Copyright (c) 2005, 2006 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*-
 * Copyright (c) 2001 Alcove - Nicolas Souchu
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: viapcib.c,v 1.6.42.1 2008/01/08 22:10:07 bouyer Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/simplelock.h>
#include <sys/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/i2c/i2cvar.h>

#include <i386/pci/viapcibreg.h>

/*#define VIAPCIB_DEBUG*/

#ifdef	VIAPCIB_DEBUG
#define	DPRINTF(x)	printf x
#else
#define	DPRINTF(x)
#endif

struct viapcib_softc {
	struct device	sc_dev;
	bus_space_tag_t	sc_iot;
	bus_space_handle_t sc_ioh;
	struct i2c_controller sc_i2c;

	int sc_revision;

	struct simplelock sc_lock;
};

static int	viapcib_match(struct device *, struct cfdata *, void *);
static void	viapcib_attach(struct device *, struct device *, void *);

static int	viapcib_clear(struct viapcib_softc *);
static int	viapcib_busy(struct viapcib_softc *);

#define		viapcib_smbus_read(sc, o) \
	bus_space_read_1((sc)->sc_iot, (sc)->sc_ioh, (o))
#define		viapcib_smbus_write(sc, o, v) \
	bus_space_write_1((sc)->sc_iot, (sc)->sc_ioh, (o), (v))

#define	VIAPCIB_SMBUS_TIMEOUT	10000

static int	viapcib_acquire_bus(void *, int);
static void	viapcib_release_bus(void *, int);
static int	viapcib_exec(void *, i2c_op_t, i2c_addr_t, const void *,
			     size_t, void *, size_t, int);

/* SMBus operations */
static int      viapcib_smbus_quick_write(void *, i2c_addr_t);
static int      viapcib_smbus_quick_read(void *, i2c_addr_t);
static int      viapcib_smbus_send_byte(void *, i2c_addr_t, uint8_t);
static int      viapcib_smbus_receive_byte(void *, i2c_addr_t,
                                           uint8_t *);
static int      viapcib_smbus_read_byte(void *, i2c_addr_t, uint8_t,
					int8_t *);
static int      viapcib_smbus_write_byte(void *, i2c_addr_t, uint8_t,
					 int8_t);
static int      viapcib_smbus_read_word(void *, i2c_addr_t, uint8_t,
					int16_t *);
static int      viapcib_smbus_write_word(void *, i2c_addr_t, uint8_t,
					 int16_t);
static int      viapcib_smbus_block_write(void *, i2c_addr_t, uint8_t,
					  int, void *);
static int      viapcib_smbus_block_read(void *, i2c_addr_t, uint8_t,
					 int, void *);
/* XXX Should be moved to smbus layer */
#define	SMB_MAXBLOCKSIZE	32

/* from arch/i386/pci/pcib.c */
extern void	pcibattach(struct device *, struct device *, void *);

CFATTACH_DECL(viapcib, sizeof(struct viapcib_softc), viapcib_match,
    viapcib_attach, NULL, NULL);

static int
viapcib_match(struct device *parent, struct cfdata *match,
    void *opaque)
{
	struct pci_attach_args *pa;

	pa = (struct pci_attach_args *)opaque;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_VIATECH)
		return 0;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_VIATECH_VT8235:
	case PCI_PRODUCT_VIATECH_VT8237:
		return 2; /* match above generic pcib(4) */
	}

	return 0;
}

static void
viapcib_attach(struct device *parent, struct device *self, void *opaque)
{
	struct viapcib_softc *sc;
	struct pci_attach_args *pa;
	pcireg_t addr, val;

	sc = (struct viapcib_softc *)self;
	pa = (struct pci_attach_args *)opaque;

	/* XXX Only the 8235 is supported for now */
	sc->sc_iot = pa->pa_iot;
	addr = pci_conf_read(pa->pa_pc, pa->pa_tag, SMB_BASE3);
	addr &= 0xfff0;
	if (bus_space_map(sc->sc_iot, addr, 8, 0, &sc->sc_ioh)) {
		printf(": failed to map SMBus I/O space\n");
		addr = 0;
		goto core_pcib;
	}

	simple_lock_init(&sc->sc_lock);

	val = pci_conf_read(pa->pa_pc, pa->pa_tag, SMB_HOST_CONFIG);
	if ((val & 1) == 0) {
		printf(": SMBus is disabled\n");
		addr = 0;
		/* XXX We can enable the SMBus here by writing 1 to
		 * SMB_HOST_CONFIG, but do we want to?
		 */
		goto core_pcib;
	}

#ifdef VIAPCIB_DEBUG
	switch (val & 0x0e) {
	case 8:
		printf(": interrupting at irq 9\n");
		break;
	case 0:
		printf(": interrupting at SMI#\n");
		break;
	default:
		printf(": interrupt misconfigured\n");
		break;
	}
#endif /* !VIAPCIB_DEBUG */

	val = pci_conf_read(pa->pa_pc, pa->pa_tag, SMB_REVISION);
	sc->sc_revision = val;

core_pcib:
	pcibattach(parent, self, opaque);

	if (addr != 0) {
		struct i2cbus_attach_args iba;
		uint8_t b;

		printf("%s: SMBus found at 0x%x (revision 0x%x)\n",
		    sc->sc_dev.dv_xname, addr, sc->sc_revision);
		
		/* Disable slave function */
		b = viapcib_smbus_read(sc, SMBSLVCNT);
		viapcib_smbus_write(sc, SMBSLVCNT, b & ~1);

		memset(&sc->sc_i2c, 0, sizeof(sc->sc_i2c));
#ifdef I2C_TYPE_SMBUS
		iba.iba_type = I2C_TYPE_SMBUS;
#endif
		iba.iba_tag = &sc->sc_i2c;
		iba.iba_tag->ic_cookie = (void *)sc;
		iba.iba_tag->ic_acquire_bus = viapcib_acquire_bus;
		iba.iba_tag->ic_release_bus = viapcib_release_bus;
		iba.iba_tag->ic_exec = viapcib_exec;

		config_found_ia(&sc->sc_dev, "i2cbus", &iba, iicbus_print);
	}

	return;
}

static int
viapcib_wait(struct viapcib_softc *sc)
{
	int rv, timeout;
	uint8_t val;

	timeout = VIAPCIB_SMBUS_TIMEOUT;
	rv = 0;

	while (timeout--) {
		val = viapcib_smbus_read(sc, SMBHSTSTS);
		if (!(val & SMBHSTSTS_BUSY) && (val & SMBHSTSTS_INTR))
			break;
		DELAY(10);
	}

	if (timeout == 0)
		rv = EBUSY;

	if ((val & SMBHSTSTS_FAILED) || (val & SMBHSTSTS_COLLISION) ||
	    (val & SMBHSTSTS_ERROR))
		rv = EIO;

	viapcib_clear(sc);

	return rv;
}

static int
viapcib_clear(struct viapcib_softc *sc)
{
	viapcib_smbus_write(sc, SMBHSTSTS,
	    (SMBHSTSTS_FAILED | SMBHSTSTS_COLLISION | SMBHSTSTS_ERROR |
	     SMBHSTSTS_INTR));
	DELAY(10);

	return 0;
}

static int
viapcib_busy(struct viapcib_softc *sc)
{
	uint8_t val;

	val = viapcib_smbus_read(sc, SMBHSTSTS);

	return (val & SMBHSTSTS_BUSY);
}

static int
viapcib_acquire_bus(void *opaque, int flags)
{
	struct viapcib_softc *sc;

	DPRINTF(("viapcib_i2c_acquire_bus(%p, 0x%x)\n", opaque, flags));

	sc = (struct viapcib_softc *)opaque;

	simple_lock(&sc->sc_lock);

	return 0;
}

static void
viapcib_release_bus(void *opaque, int flags)
{
	struct viapcib_softc *sc;

	DPRINTF(("viapcib_i2c_release_bus(%p, 0x%x)\n", opaque, flags));

	sc = (struct viapcib_softc *)opaque;

	simple_unlock(&sc->sc_lock);

	return;
}

static int
viapcib_exec(void *opaque, i2c_op_t op, i2c_addr_t addr, const void *vcmd,
    size_t cmdlen, void *vbuf, size_t buflen, int flags)
{
	struct viapcib_softc *sc;
	uint8_t cmd;
	int rv = -1;

	DPRINTF(("viapcib_exec(%p, 0x%x, 0x%x, %p, %d, %p, %d, 0x%x)\n",
	    opaque, op, addr, vcmd, cmdlen, vbuf, buflen, flags));

	sc = (struct viapcib_softc *)opaque;

	if (op != I2C_OP_READ_WITH_STOP &&
	    op != I2C_OP_WRITE_WITH_STOP)
		return -1;

	if (cmdlen > 0)
		cmd = *(uint8_t *)(__UNCONST(vcmd)); /* XXX */

	switch (cmdlen) {
	case 0:
		switch (buflen) {
		case 0:
			/* SMBus quick read/write */
			if (I2C_OP_READ_P(op))
				rv = viapcib_smbus_quick_read(sc, addr);
			else
				rv = viapcib_smbus_quick_write(sc, addr);

			return rv;
		case 1:
			/* SMBus send/receive byte */
			if (I2C_OP_READ_P(op))
				rv = viapcib_smbus_send_byte(sc, addr,
				    *(uint8_t *)vbuf);
			else
				rv = viapcib_smbus_receive_byte(sc, addr,
				    (uint8_t *)vbuf);
			return rv;
		default:
			return -1;
		}
	case 1:
		switch (buflen) {
		case 0:
			return -1;
		case 1:
			/* SMBus read/write byte */
			if (I2C_OP_READ_P(op))
				rv = viapcib_smbus_read_byte(sc, addr,
				    cmd, (uint8_t *)vbuf);
			else
				rv = viapcib_smbus_write_byte(sc, addr,
				    cmd, *(uint8_t *)vbuf);
			return rv;
		case 2:
			/* SMBus read/write word */
			if (I2C_OP_READ_P(op))
				rv = viapcib_smbus_read_word(sc, addr,
				    cmd, (uint16_t *)vbuf);
			else
				rv = viapcib_smbus_write_word(sc, addr,
				    cmd, *(uint16_t *)vbuf);
			return rv;
		default:
			/* SMBus read/write block */
			if (I2C_OP_READ_P(op))
				rv = viapcib_smbus_block_read(sc, addr,
				    cmd, buflen, vbuf);
			else
				rv = viapcib_smbus_block_write(sc, addr,
				    cmd, buflen, vbuf);
			return rv;
		}
	}

	return -1; /* XXX ??? */
}

static int
viapcib_smbus_quick_write(void *opaque, i2c_addr_t slave)
{
	struct viapcib_softc *sc;

	sc = (struct viapcib_softc *)opaque;

	DPRINTF(("viapcib_smbus_quick_write(%p, 0x%x)\n", opaque, slave));

	viapcib_clear(sc);
	if (viapcib_busy(sc))
		return EBUSY;

	viapcib_smbus_write(sc, SMBHSTADD, (slave & 0x7f) << 1);
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_QUICK);
	if (viapcib_wait(sc))
		return EBUSY;
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_QUICK | SMBHSTCNT_START);

	return viapcib_wait(sc);
}

static int
viapcib_smbus_quick_read(void *opaque, i2c_addr_t slave)
{
	struct viapcib_softc *sc;

	sc = (struct viapcib_softc *)opaque;

	DPRINTF(("viapcib_smbus_quick_read(%p, 0x%x)\n", opaque, slave));

	viapcib_clear(sc);
	if (viapcib_busy(sc))
		return EBUSY;

	viapcib_smbus_write(sc, SMBHSTADD, ((slave & 0x7f) << 1) | 1);
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_QUICK);
	if (viapcib_wait(sc))
		return EBUSY;
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_QUICK | SMBHSTCNT_START);

	return viapcib_wait(sc);
}

static int
viapcib_smbus_send_byte(void *opaque, i2c_addr_t slave, uint8_t byte)
{
	struct viapcib_softc *sc;

	sc = (struct viapcib_softc *)opaque;

	DPRINTF(("viapcib_smbus_send_byte(%p, 0x%x, 0x%x)\n", opaque,
	    slave, byte));

	viapcib_clear(sc);
	if (viapcib_busy(sc))
		return EBUSY;

	viapcib_smbus_write(sc, SMBHSTADD, (slave & 0x7f) << 1);
	viapcib_smbus_write(sc, SMBHSTCMD, byte);
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_SENDRECV);
	if (viapcib_wait(sc))
		return EBUSY;
	viapcib_smbus_write(sc, SMBHSTCNT,
	    SMBHSTCNT_START | SMBHSTCNT_SENDRECV);

	return viapcib_wait(sc);
}

static int
viapcib_smbus_receive_byte(void *opaque, i2c_addr_t slave, uint8_t *pbyte)
{
	struct viapcib_softc *sc;
	int rv;

	sc = (struct viapcib_softc *)opaque;

	DPRINTF(("viapcib_smbus_receive_byte(%p, 0x%x, %p)\n", opaque,
	    slave, pbyte));

	viapcib_clear(sc);
	if (viapcib_busy(sc))
		return EBUSY;

	viapcib_smbus_write(sc, SMBHSTADD, ((slave & 0x7f) << 1) | 1);
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_SENDRECV);
	if (viapcib_wait(sc))
		return EBUSY;
	viapcib_smbus_write(sc, SMBHSTCNT,
	    SMBHSTCNT_START | SMBHSTCNT_SENDRECV);

	rv = viapcib_wait(sc);
	if (rv == 0)
		*pbyte = viapcib_smbus_read(sc, SMBHSTDAT0);

	return rv;
}

static int
viapcib_smbus_write_byte(void *opaque, i2c_addr_t slave, uint8_t cmd,
		   int8_t byte)
{
	struct viapcib_softc *sc;

	sc = (struct viapcib_softc *)opaque;

	DPRINTF(("viapcib_smbus_write_byte(%p, 0x%x, 0x%x, 0x%x)\n", opaque,
	    slave, cmd, byte));

	viapcib_clear(sc);
	if (viapcib_busy(sc))
		return EBUSY;

	viapcib_smbus_write(sc, SMBHSTADD, (slave & 0x7f) << 1);
	viapcib_smbus_write(sc, SMBHSTCMD, cmd);
	viapcib_smbus_write(sc, SMBHSTDAT0, byte);
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_BYTE);
	if (viapcib_wait(sc))
		return EBUSY;
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_START | SMBHSTCNT_BYTE);

	return viapcib_wait(sc);
}

static int
viapcib_smbus_read_byte(void *opaque, i2c_addr_t slave, uint8_t cmd,
		  int8_t *pbyte)
{
	struct viapcib_softc *sc;
	int rv;

	sc = (struct viapcib_softc *)opaque;

	DPRINTF(("viapcib_smbus_read_byte(%p, 0x%x, 0x%x, %p)\n", opaque,
	    slave, cmd, pbyte));

	viapcib_clear(sc);
	if (viapcib_busy(sc))
		return EBUSY;

	viapcib_smbus_write(sc, SMBHSTADD, ((slave & 0x7f) << 1) | 1);
	viapcib_smbus_write(sc, SMBHSTCMD, cmd);
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_BYTE);
	if (viapcib_wait(sc))
		return EBUSY;
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_START | SMBHSTCNT_BYTE);
	rv = viapcib_wait(sc);
	if (rv == 0)
		*pbyte = viapcib_smbus_read(sc, SMBHSTDAT0);

	return rv;
}

static int
viapcib_smbus_write_word(void *opaque, i2c_addr_t slave, uint8_t cmd,
		   int16_t word)
{
	struct viapcib_softc *sc;

	sc = (struct viapcib_softc *)opaque;

	DPRINTF(("viapcib_smbus_write_word(%p, 0x%x, 0x%x, 0x%x)\n", opaque,
	    slave, cmd, word));

	viapcib_clear(sc);
	if (viapcib_busy(sc))
		return EBUSY;

	viapcib_smbus_write(sc, SMBHSTADD, (slave & 0x7f) << 1);
	viapcib_smbus_write(sc, SMBHSTCMD, cmd);
	viapcib_smbus_write(sc, SMBHSTDAT0, word & 0x00ff);
	viapcib_smbus_write(sc, SMBHSTDAT1, (word & 0xff00) >> 8);
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_WORD);
	if (viapcib_wait(sc))
		return EBUSY;
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_START | SMBHSTCNT_WORD);

	return viapcib_wait(sc);
}

static int
viapcib_smbus_read_word(void *opaque, i2c_addr_t slave, uint8_t cmd,
		  int16_t *pword)
{
	struct viapcib_softc *sc;
	int rv;
	int8_t high, low;

	sc = (struct viapcib_softc *)opaque;

	DPRINTF(("viapcib_smbus_read_word(%p, 0x%x, 0x%x, %p)\n", opaque,
	    slave, cmd, pword));

	viapcib_clear(sc);
	if (viapcib_busy(sc))
		return EBUSY;

	viapcib_smbus_write(sc, SMBHSTADD, ((slave & 0x7f) << 1) | 1);
	viapcib_smbus_write(sc, SMBHSTCMD, cmd);
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_WORD);
	if (viapcib_wait(sc))
		return EBUSY;
	viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_START | SMBHSTCNT_WORD);

	rv = viapcib_wait(sc);
	if (rv == 0) {
		low = viapcib_smbus_read(sc, SMBHSTDAT0);
		high = viapcib_smbus_read(sc, SMBHSTDAT1);
		*pword = ((high & 0xff) << 8) | (low & 0xff);
	}

	return rv;
}

static int
viapcib_smbus_block_write(void *opaque, i2c_addr_t slave, uint8_t cmd,
		    int cnt, void *data)
{
	struct viapcib_softc *sc;
	int8_t *buf;
	int remain, len, i;
	int rv;

	sc = (struct viapcib_softc *)opaque;
	buf = (int8_t *)data;
	rv = 0;

	DPRINTF(("viapcib_smbus_block_write(%p, 0x%x, 0x%x, %d, %p)\n",
	    opaque, slave, cmd, cnt, data));

	viapcib_clear(sc);
	if (viapcib_busy(sc))
		return EBUSY;

	remain = cnt;
	while (remain) {
		len = min(remain, SMB_MAXBLOCKSIZE);

		viapcib_smbus_write(sc, SMBHSTADD, (slave & 0x7f) << 1);
		viapcib_smbus_write(sc, SMBHSTCMD, cmd);
		viapcib_smbus_write(sc, SMBHSTDAT0, len);
		i = viapcib_smbus_read(sc, SMBHSTCNT);
		/* XXX FreeBSD does this, but it looks wrong */
		for (i = 0; i < len; i++) {
			viapcib_smbus_write(sc, SMBBLKDAT,
			    buf[cnt - remain + i]);
		}
		viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_BLOCK);
		if (viapcib_wait(sc))
			return EBUSY;
		viapcib_smbus_write(sc, SMBHSTCNT,
		    SMBHSTCNT_START | SMBHSTCNT_BLOCK);
		if (viapcib_wait(sc))
			return EBUSY;

		remain -= len;
	}

	return rv;
}

static int
viapcib_smbus_block_read(void *opaque, i2c_addr_t slave, uint8_t cmd,
			 int cnt, void *data)
{
	struct viapcib_softc *sc;
	int8_t *buf;
	int remain, len, i;
	int rv;

	sc = (struct viapcib_softc *)opaque;
	buf = (int8_t *)data;
	rv = 0;

	DPRINTF(("viapcib_smbus_block_read(%p, 0x%x, 0x%x, %d, %p)\n",
	    opaque, slave, cmd, cnt, data));

	viapcib_clear(sc);
	if (viapcib_busy(sc))
		return EBUSY;

	remain = cnt;
	while (remain) {
		viapcib_smbus_write(sc, SMBHSTADD, ((slave & 0x7f) << 1) | 1);
		viapcib_smbus_write(sc, SMBHSTCMD, cmd);
		viapcib_smbus_write(sc, SMBHSTCNT, SMBHSTCNT_BLOCK);
		if (viapcib_wait(sc))
			return EBUSY;
		viapcib_smbus_write(sc, SMBHSTCNT,
		    SMBHSTCNT_START | SMBHSTCNT_BLOCK);
		if (viapcib_wait(sc))
			return EBUSY;

		len = viapcib_smbus_read(sc, SMBHSTDAT0);
		i = viapcib_smbus_read(sc, SMBHSTCNT);

		len = min(len, remain);

		/* FreeBSD does this too... */
		for (i = 0; i < len; i++) {
			buf[cnt - remain + i] =
			    viapcib_smbus_read(sc, SMBBLKDAT);
		}

		remain -= len;
	}

	return rv;
}
