/*	$NetBSD: iic.c,v 1.1 2003/09/23 14:56:08 shige Exp $	*/

/*
 * Copyright (c) 2003 Shigeyuki Fukushima.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iic.c,v 1.1 2003/09/23 14:56:08 shige Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <powerpc/ibm4xx/dcr405gp.h>
#include <powerpc/ibm4xx/ibm405gp.h>
#include <powerpc/ibm4xx/dev/opbvar.h>
#include <powerpc/ibm4xx/dev/iicreg.h>
#include <powerpc/ibm4xx/dev/iicvar.h>

#define IIC_READ(sc, reg) \
	bus_space_read_1(sc->sc_st, sc->sc_sh, (reg))
#define IIC_WRITE(sc, reg, val) \
	bus_space_write_1(sc->sc_st, sc->sc_sh, (reg), (val))

/*
 * XXX: builtin devices
 */
#define	NVRAM_BASE	0xf0000000
const struct iicbus_dev {
	const char *name;
	bus_addr_t addr;
	int irq;
} iicbus_devs[] = {
	{ "rtc",	NVRAM_BASE,	-1 },
	{ NULL }
};

static int	iic_match(struct device *, struct cfdata *, void *);
static void	iic_attach(struct device *, struct device *, void *);
static int	iic_submatch(struct device *, struct cfdata *, void *);
static int	iic_print(void *, const char *);

static void	iic_reset(struct iic_softc *sc);
static int	iic_master_xfer(struct iic_softc *sc,
			int rpst_flag, int rw_flag,
			u_int16_t addr, int addrtype,
			u_char *data, int len);
static int	iic_master_read_bytes(struct iic_softc *sc,
			u_char *buf, int count, int rpst_flag);
static int	iic_master_write_bytes(struct iic_softc *sc,
			u_char *buf, int count, int rpst_flag);
static void	iic_xfer_poll(struct iic_softc *sc);
static int	iic_err_status(struct iic_softc *sc);

#ifdef IIC_DEBUG
static void   iic_print_register(struct iic_softc *sc);
#endif

CFATTACH_DECL(iic, sizeof(struct device),
    iic_match, iic_attach, NULL, NULL);

/*
 * Probe for the IIC.
 */
static int
iic_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct opb_attach_args *oaa = aux;

	/* match only on-chip iic devices */
	if (strcmp(oaa->opb_name, cf->cf_name) == 0)
		return (1);

	return (0);
}

static int
iic_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct iicbus_attach_args *iaa = aux;

	if (cf->cf_loc[IICCF_ADDR] != IICCF_ADDR_DEFAULT &&
	    cf->cf_loc[IICCF_ADDR] != iaa->iicb_addr)
		return (0);

	return (config_match(parent, cf, aux));
}

/*
 * Attach the IIC.
 */
static void
iic_attach(struct device *parent, struct device *self, void *aux)
{
	struct opb_attach_args *oaa = aux;
	struct iicbus_attach_args iaa;
	struct iic_softc *sc = (struct iic_softc *)self;
	int i;

	sc->sc_st = oaa->opb_bt;
	sc->sc_sh = oaa->opb_addr;
	sc->sc_dmat = oaa->opb_dmat;

	printf(": 405GP IIC\n");

	/*
	 * Reset the chip to a known state.
	 */
	iic_reset(sc);

#if 0
	{
		/* test code */
		u_char sr, buf[2];

		buf[0] = 0x00;
		buf[1] = 0x37;

		iic_master_xfer(sc, IIC_XFER_RPTST, IIC_XFER_WRITE,
			0x6f, IIC_ADM_7BIT, &buf[0],  2);

		iic_master_xfer(sc, IIC_XFER_NRMST, IIC_XFER_READ,
			0x6f, IIC_ADM_7BIT, &sr, 1);

		printf("SR(0x%x): 0x%x\n", buf[1], sr);
	}
#endif

	for (i = 0 ; iicbus_devs[i].name != NULL ; i++) {
		iaa.iicb_name = iicbus_devs[i].name;
		iaa.iicb_addr = iicbus_devs[i].addr;
		iaa.iicb_irq = iicbus_devs[i].irq;
		iaa.iicb_bt = oaa->opb_bt;
		iaa.iicb_dmat = oaa->opb_dmat;
		iaa.adap = sc;

		(void) config_found_sm(self, &iaa, iic_print, iic_submatch);
	}

	return;
}

static int
iic_print(void *aux, const char *pnp)
{
	struct iicbus_attach_args *iaa = aux;

	if (pnp)
		printf("%s at %s", iaa->iicb_name, pnp);
	if (iaa->iicb_addr != IICCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%08lx", iaa->iicb_addr);
	if (iaa->iicb_irq != IICCF_IRQ_DEFAULT)
		aprint_normal(" irq %d", iaa->iicb_irq);

	return (UNCONF);
}

/*
 * iic_read()
 */
int iic_read(struct iic_softc *sc, u_char saddr,
	u_char *addr, int alen, u_char *data, int dlen)
{
	int ret;

	ret = iic_master_xfer(sc, IIC_XFER_RPTST, IIC_XFER_WRITE,
		saddr, IIC_ADM_7BIT, addr,  alen);

	ret = iic_master_xfer(sc, IIC_XFER_NRMST, IIC_XFER_READ,
		saddr, IIC_ADM_7BIT, data, dlen);

	return (ret);
}

/*
 * iic_write()
 */
int iic_write(struct iic_softc *sc, u_char saddr,
	u_char *addr, int alen, u_char *data, int dlen)
{
	int ret;
	u_char d[alen + dlen];

	memcpy(d, addr, alen);
	memcpy(d+alen, data, dlen);
	ret = iic_master_xfer(sc, IIC_XFER_NRMST, IIC_XFER_WRITE,
		saddr, IIC_ADM_7BIT, d, (alen + dlen));

	return (ret);
}

/*
 * iic_reset()
 * Perform IIC Soft Reset
 */
static void
iic_reset(struct iic_softc *sc)
{
	/* Clear Low/High Master Address */
	IIC_WRITE(sc, IIC_LMADR, 0x0);
	IIC_WRITE(sc, IIC_HMADR, 0x0);

	/* Clear Low/High Slave Address */
	IIC_WRITE(sc, IIC_LSADR, 0x0);
	IIC_WRITE(sc, IIC_HSADR, 0x0);

	/* Clear any active interrupts */
	IIC_WRITE(sc, IIC_STS, IIC_STS_SCMP | IIC_STS_IRQA);

	/* Clear all writable status bits */
	IIC_WRITE(sc, IIC_EXTSTS, IIC_EXTSTS_IRQP
	 | IIC_EXTSTS_IRQD
	 | IIC_EXTSTS_LA
	 | IIC_EXTSTS_ICT
	 | IIC_EXTSTS_XFRA);

	/* Clear all writable status bits */
	IIC_WRITE(sc, IIC_CLKDIV, IIC_CLKDIV_70MHZ); /* XXX: OBS266 66MHz */

	/* Clear all interrupt mask bits */
	/* IIC_WRITE(sc, IIC_INTRMSK, IIC_INTRMSK_EIMTC); */
	IIC_WRITE(sc, IIC_INTRMSK, 0x0);

	/* Clear Transfer Counter */
	IIC_WRITE(sc, IIC_XFRCNT, 0x0);

	/* Clear status set */
	IIC_WRITE(sc, IIC_XTCNTLSS, IIC_XTCNTLSS_SRC
	 | IIC_XTCNTLSS_SRS
	 | IIC_XTCNTLSS_SWC
	 | IIC_XTCNTLSS_SWS);

	/*
	 * Initialize Mode Control
	 *   Flushes master and slave data buffers
	 *   Enables IIC Standard mode (100Kb clock)
	 *   Disables Interrupts
	 *   Disables Slave Mode
	 *   Enables Interrupt
	 *   Enables Exit Unknown State
	 *   Hold IIC Serial Clock Low
	 */
	IIC_WRITE(sc, IIC_MDCNTL, IIC_MDCNTL_FMDB
	 | IIC_MDCNTL_FSDB
	 | IIC_MDCNTL_EUBS);

	IIC_WRITE(sc, IIC_CNTL, 0x0);

#ifdef IIC_DEBUG
	iic_print_register(sc);
#endif

	return;
}

/*
 * iic_master_xfer()
 * Perform IIC read/write single transfer
 */
static int
iic_master_xfer(struct iic_softc *sc, int rpst_flag, int rw_flag,
	u_int16_t addr, int addrtype, u_char *data, int len)
{
	int ret = 0;
	u_char r, l, h;

	/*
	 * starting transfer init
	 */
	/* Request to halt transfer */
	IIC_WRITE(sc, IIC_STS, IIC_STS_SCMP);
	/* Wait for any pending transfer */
	while (IIC_READ(sc, IIC_STS) & IIC_STS_PT) ;
	/* Flush master data buffer */
	r = IIC_READ(sc, IIC_MDCNTL);
	IIC_WRITE(sc, IIC_MDCNTL, r | IIC_MDCNTL_FMDB);

	/* Writing slave address */
	switch (addrtype) {
	case IIC_ADM_10BIT:
		l = addr & 0xff;
		h = 0xf0 | (((addr >> 8) & 0x03) << IIC_HADR_SHFT);
		break;
	case IIC_ADM_7BIT:
	default:
		l = (addr << IIC_LADR_SHFT) & 0xff;
		h = 0x00;
		break;
	}
	IIC_WRITE(sc, IIC_HMADR, h);
	IIC_WRITE(sc, IIC_LMADR, l);

	/* Check if the bus is busy */
	r = IIC_READ(sc, IIC_EXTSTS);
	r &= IIC_EXTSTS_BCS;
	/*
	if (r != IIC_EXTSTS_BCS_FREE) {
		return IIC_ERR_NOBUSFREE;
	}
	*/

	if (rw_flag == IIC_XFER_READ) {
		ret = iic_master_read_bytes(sc, data, len, rpst_flag);
	} else {
		ret = iic_master_write_bytes(sc, data, len, rpst_flag);
	}

	return ret;
}

/*
 * iic_master_read_bytes()
 * Perform IIC master read
 */
static int
iic_master_read_bytes(struct iic_softc *sc,
	u_char *buf, int count, int rpst_flag)
{
	int i, j, ret;
	int loops, remains, rcount = 0;
	u_char r, tct, rpst = 0;

	if (count == 0) {
		return 0;
	}
	loops = count / IIC_FIFO_BUFSIZE;
	remains = count % IIC_FIFO_BUFSIZE;

	/* checking last loops */
	if ((loops > 1) && (remains == 0)) {
		loops = loops - 1;
		remains = IIC_FIFO_BUFSIZE;
	}

	/* transfer chain loop: 0 .. (loops -1) */
	for (i = 0 ; i < loops ; i++) {
		tct = IIC_FIFO_BUFSIZE - 1;
		tct = (tct << IIC_CNTL_TCT_SHFT) & IIC_CNTL_TCT;
 	 	/*
		 * Start transfer: transfer count | Chain | Read | Begin
		 */
		r = tct | IIC_CNTL_CHT | IIC_CNTL_RW | IIC_CNTL_PT;
#ifdef IIC_DEBUG
		printf("Start Transfer(IIC_CNTL=0x%02x)\n", r);
#endif
		IIC_WRITE(sc, IIC_CNTL, r);
		iic_xfer_poll(sc);
		ret = iic_err_status(sc);
		if (ret == 1) {
			return rcount;
		}
		ret = IIC_READ(sc, IIC_XFRCNT) & IIC_XFRCNT_MTC;
#ifdef IIC_DEBUG
		printf("Read from MDBUF:");
#endif
		for (j = 0 ; j < ret ; j++) {
			delay(1);
			buf[rcount] = IIC_READ(sc, IIC_MDBUF);
#ifdef IIC_DEBUG
			printf(" 0x%02x\n", buf[rcount]);
#endif
			rcount++;
		}
#ifdef IIC_DEBUG
		printf("\n");
#endif
	}

	/* last loops */
	tct = remains - 1;
	tct = (tct << IIC_CNTL_TCT_SHFT) & IIC_CNTL_TCT;
	if (rpst_flag == IIC_XFER_RPTST) {
		rpst  = IIC_CNTL_RPST;
	}
	r = tct | rpst | IIC_CNTL_RW | IIC_CNTL_PT;
#ifdef IIC_DEBUG
	printf("Start Transfer(IIC_CNTL=0x%02x)\n", r);
#endif
	IIC_WRITE(sc, IIC_CNTL, r);
	iic_xfer_poll(sc);
	ret = iic_err_status(sc);
	if (ret == 1) {
		return rcount;
	}
	ret = IIC_READ(sc, IIC_XFRCNT) & IIC_XFRCNT_MTC;
#ifdef IIC_DEBUG
	printf("Read from MDBUF:");
#endif
	for (j = 0 ; j < ret ; j++) {
		delay(1);
		buf[rcount] = IIC_READ(sc, IIC_MDBUF);
#ifdef IIC_DEBUG
		printf(" 0x%02x", buf[rcount]);
#endif
		rcount++;
	}
#ifdef IIC_DEBUG
	printf("\n");
#endif

	return rcount;
}

/*
 * iic_master_write_bytes()
 * Perform IIC master write
 */
static int
iic_master_write_bytes(struct iic_softc *sc,
	u_char *buf, int count, int rpst_flag)
{
	int i, j, ret;
	int loops, remains, wcount = 0;
	u_char r, tct, rpst = 0;

	if (count == 0) {
		return 0;
	}
	loops = count / IIC_FIFO_BUFSIZE;
	remains = count % IIC_FIFO_BUFSIZE;

	/* checking last loops */
	if ((loops > 1) && (remains == 0)) {
		loops = loops - 1;
		remains = IIC_FIFO_BUFSIZE;
	}

	/* transfer chain loop: 0 .. (loops -1) */
	for (i = 0 ; i < loops ; i++) {
#ifdef IIC_DEBUG
		printf("Write to MDBUF:");
#endif
		for (j = 0 ; j < IIC_FIFO_BUFSIZE ; j++) {
			IIC_WRITE(sc, IIC_MDBUF, buf[wcount]);
#ifdef IIC_DEBUG
			printf(" 0x%02x", buf[wcount]);
#endif
			wcount++;
		}
#ifdef IIC_DEBUG
		printf("\n");
#endif

		tct = IIC_FIFO_BUFSIZE - 1;
		tct = (tct << IIC_CNTL_TCT_SHFT) & IIC_CNTL_TCT;
 	 	/*
		 * Start transfer: transfer count | Chain | Begin
		 */
		r = tct | IIC_CNTL_CHT | IIC_CNTL_PT;
#ifdef IIC_DEBUG
		printf("Start Transfer(IIC_CNTL=0x%02x)\n", r);
#endif
		IIC_WRITE(sc, IIC_CNTL, r);
		iic_xfer_poll(sc);
		ret = iic_err_status(sc);
		if (ret == 1) {
			wcount -= IIC_FIFO_BUFSIZE;
			wcount += IIC_READ(sc, IIC_XFRCNT) & IIC_XFRCNT_MTC;
			return wcount;
		}
	}

	/* last loops */
#ifdef IIC_DEBUG
	printf("Write to MDBUF:");
#endif
	for (j = 0 ; j < remains ; j++) {
		IIC_WRITE(sc, IIC_MDBUF, buf[wcount]);
#ifdef IIC_DEBUG
		printf(" 0x%02x", buf[wcount]);
#endif
		wcount++;
	}
#ifdef IIC_DEBUG
	printf("\n");
#endif
	tct = remains - 1;
	tct = (tct << IIC_CNTL_TCT_SHFT) & IIC_CNTL_TCT;
	if (rpst_flag == IIC_XFER_RPTST) {
		rpst = IIC_CNTL_RPST;
	}
	r = tct | rpst | IIC_CNTL_PT;
#ifdef IIC_DEBUG
	printf("Start Transfer(IIC_CNTL=0x%02x)\n", r);
#endif
	IIC_WRITE(sc, IIC_CNTL, r);
	iic_xfer_poll(sc);
	ret = iic_err_status(sc);
	if (ret == 1) {
		wcount -= remains;
		wcount += IIC_READ(sc, IIC_XFRCNT) & IIC_XFRCNT_MTC;
		return wcount;
	}

	return wcount;
}

/*
 * iic_xfer_poll()
 * Perform IIC transfer complete polling
 */
static void
iic_xfer_poll(struct iic_softc *sc)
{
	while (IIC_READ(sc, IIC_STS) & IIC_STS_PT) {
		;
	}
	/* XXX: timeout */
}

/*
 * iic_err_status()
 * Perform IIC checking error status
 */
static int
iic_err_status(struct iic_softc *sc)
{
	/* Read the Error status bit */
	if (IIC_READ(sc, IIC_STS) & IIC_STS_ERR) {
		/* XXX Recover */
		printf("IIC_STS:    0x%x\n", IIC_STS);
		printf("IIC_EXTSTS: 0x%x\n", IIC_EXTSTS);
		return (1);
	}
	return (0);
}

#ifdef IIC_DEBUG
static void
iic_print_register(struct iic_softc *sc)
{
	u_char mdbuf, sdbuf, lmadr, hmadr, cntl, mdcntl;
	u_char sts, extsts, lsadr, hsadr, clkdiv, intrmsk, xfrcnt, xtcntlss;
	u_char dcntl;

	clkdiv = IIC_READ(sc, IIC_CLKDIV);
	cntl = IIC_READ(sc, IIC_CNTL);
	dcntl = IIC_READ(sc, IIC_DIRECTCNTL);
	extsts = IIC_READ(sc, IIC_EXTSTS);
	hmadr = IIC_READ(sc, IIC_HMADR);
	hsadr = IIC_READ(sc, IIC_HSADR);
	intrmsk = IIC_READ(sc, IIC_INTRMSK);
	lmadr = IIC_READ(sc, IIC_LMADR);
	lsadr = IIC_READ(sc, IIC_LSADR);
	mdbuf = IIC_READ(sc, IIC_MDBUF);
	mdcntl = IIC_READ(sc, IIC_MDCNTL);
	sdbuf = IIC_READ(sc, IIC_SDBUF);
	sts = IIC_READ(sc, IIC_STS);
	xfrcnt = IIC_READ(sc, IIC_XFRCNT);
	xtcntlss = IIC_READ(sc, IIC_XTCNTLSS);

	printf("IIC Registers (0x%08x):\n", sc->sc_sh);
	printf("0x00:  0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
		mdbuf, 0, sdbuf, 0, lmadr, hmadr, cntl, mdcntl);
	printf("0x08:  0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
		sts, extsts, lsadr, hsadr, clkdiv, intrmsk, xfrcnt, xtcntlss);
	printf("0x10:  0x%02x\n", dcntl);

	return;
}
#endif
