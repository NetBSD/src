/*	$NetBSD: scimci.c,v 1.1.4.2 2010/05/30 05:16:48 rmind Exp $	*/

/*-
 * Copyright (c) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Serial Peripheral interface driver to access MMC card
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: scimci.c,v 1.1.4.2 2010/05/30 05:16:48 rmind Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <sh3/devreg.h>
#include <sh3/pfcreg.h>
#include <sh3/scireg.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>

#include <evbsh3/t_sh7706lan/t_sh7706lanvar.h>

#ifdef SCIMCI_DEBUG
int scimci_debug = 1;
#define DPRINTF(n,s)	do { if ((n) <= scimci_debug) printf s; } while (0)
#else
#define DPRINTF(n,s)	do {} while (0)
#endif

static int	scimci_host_reset(sdmmc_chipset_handle_t);
static uint32_t	scimci_host_ocr(sdmmc_chipset_handle_t);
static int	scimci_host_maxblklen(sdmmc_chipset_handle_t);
static int	scimci_card_detect(sdmmc_chipset_handle_t);
static int	scimci_write_protect(sdmmc_chipset_handle_t);
static int	scimci_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	scimci_bus_clock(sdmmc_chipset_handle_t, int);
static int	scimci_bus_width(sdmmc_chipset_handle_t, int);
static void	scimci_exec_command(sdmmc_chipset_handle_t,
		    struct sdmmc_command *);

static struct sdmmc_chip_functions scimci_chip_functions = {
	/* host controller reset */
	.host_reset		= scimci_host_reset,

	/* host controller capabilities */
	.host_ocr		= scimci_host_ocr,
	.host_maxblklen		= scimci_host_maxblklen,

	/* card detection */
	.card_detect		= scimci_card_detect,

	/* write protect */
	.write_protect		= scimci_write_protect,

	/* bus power, clock frequency, width */
	.bus_power		= scimci_bus_power,
	.bus_clock		= scimci_bus_clock,
	.bus_width		= scimci_bus_width,

	/* command execution */
	.exec_command		= scimci_exec_command,

	/* card interrupt */
	.card_enable_intr	= NULL,
	.card_intr_ack		= NULL,
};

static void	scimci_spi_initialize(sdmmc_chipset_handle_t);

static struct sdmmc_spi_chip_functions scimci_spi_chip_functions = {
	.initialize		= scimci_spi_initialize,
};

#define	CSR_SET_1(reg,set,mask) 					\
do {									\
	uint8_t _r;							\
	_r = _reg_read_1((reg));					\
	_r &= ~(mask);							\
	_r |= (set);							\
	_reg_write_1((reg), _r);					\
} while (/*CONSTCOND*/0)

#define	CSR_SET_2(reg,set,mask) 					\
do {									\
	uint16_t _r;							\
	_r = _reg_read_2((reg));					\
	_r &= ~(mask);							\
	_r |= (set);							\
	_reg_write_2((reg), _r);					\
} while (/*CONSTCOND*/0)

#define	CSR_CLR_1(reg,clr)	 					\
do {									\
	uint8_t _r;							\
	_r = _reg_read_1((reg));					\
	_r &= ~(clr);							\
	_reg_write_1((reg), _r);					\
} while (/*CONSTCOND*/0)

#define	CSR_CLR_2(reg,clr)	 					\
do {									\
	uint16_t _r;							\
	_r = _reg_read_2((reg));					\
	_r &= ~(clr);							\
	_reg_write_2((reg), _r);					\
} while (/*CONSTCOND*/0)

#define SCPCR_CLK_MASK	0x000C
#define SCPCR_CLK_IN	0x000C
#define SCPCR_CLK_OUT	0x0004
#define SCPDR_CLK	0x02
#define SCPCR_DAT_MASK	0x0003
#define SCPCR_DAT_IN	0x0003
#define SCPCR_DAT_OUT	0x0001
#define SCPDR_DAT	0x01
#define SCPCR_CMD_MASK	0x0030
#define SCPCR_CMD_IN	0x0030
#define SCPCR_CMD_OUT	0x0010
#define SCPDR_CMD	0x04
#define SCPCR_CS_MASK	0x00C0
#define SCPCR_CS_IN	0x00C0
#define SCPCR_CS_OUT	0x0040
#define SCPDR_CS	0x08
#define PGCR_EJECT	0x0300
#define PGDR_EJECT	0x10

/* SCSCR */
#define SCSCR_SCK_OUT	0
#define SCSCR_SCK_IN	(SCSCR_CKE1)

#define LOW_SPEED	144
#define MID_SPEED	48
#define MMC_TIME_OVER	1000

struct scimci_softc {
	device_t sc_dev;
	device_t sc_sdmmc;
};

static int scimci_match(device_t, cfdata_t, void *);
static void scimci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(scimci, sizeof(struct scimci_softc),
    scimci_match, scimci_attach, NULL, NULL);

static void scimci_putc(int);
static void scimci_putc_sw(void);
static int scimci_getc(void);
static void scimci_getc_sw(void);
static void scimci_cmd_cfgread(struct scimci_softc *, struct sdmmc_command *);
static void scimci_cmd_read(struct scimci_softc *, struct sdmmc_command *);
static void scimci_cmd_write(struct scimci_softc *, struct sdmmc_command *);

void scimci_read_buffer(u_char *buf);
void scimci_write_buffer(const u_char *buf);

/*ARGSUSED*/
static int
scimci_match(device_t parent, cfdata_t cf, void *aux)
{

	if (IS_SH7706LSR)
		return 0;
	return 1;
}

/*ARGSUSED*/
static void
scimci_attach(device_t parent, device_t self, void *aux)
{
	struct scimci_softc *sc = device_private(self);
	struct sdmmcbus_attach_args saa;

	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal(": SCI MMC controller\n");

	/* Setup */
	CSR_SET_2(SH7709_SCPCR, SCPCR_CLK_OUT | SCPCR_CMD_OUT,
	    SCPCR_CLK_MASK | SCPCR_CMD_MASK);
	CSR_CLR_2(SH7709_SCPCR, SCPCR_CMD_MASK);
	CSR_SET_1(SH7709_SCPDR, SCPDR_CLK | SCPDR_CMD, 0);
	CSR_SET_2(SH7709_SCPCR, SCPCR_CS_OUT, SCPCR_CS_MASK);

	SHREG_SCSCR = 0x00;
	SHREG_SCSSR = 0x00;
	SHREG_SCSCMR = 0xfa;	/* MSB first */
	SHREG_SCSMR = SCSMR_CA;	/* clock sync mode */
	SHREG_SCBRR = LOW_SPEED;
	delay(1000);		/* wait 1ms */

	/*
	 * Attach the generic SD/MMC bus driver.  (The bus driver must
	 * not invoke any chipset functions before it is attached.)
	 */
	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &scimci_chip_functions;
	saa.saa_spi_sct = &scimci_spi_chip_functions;
	saa.saa_sch = sc;
	saa.saa_clkmin = 4000 / (LOW_SPEED + 1);
	saa.saa_clkmax = 4000 / (MID_SPEED + 1);
	saa.saa_caps = SMC_CAPS_SPI_MODE
		     | SMC_CAPS_SINGLE_ONLY
		     | SMC_CAPS_POLL_CARD_DET;

	sc->sc_sdmmc = config_found(sc->sc_dev, &saa, NULL);
	if (sc->sc_sdmmc == NULL)
		aprint_error_dev(sc->sc_dev, "couldn't attach bus\n");
}

/*
 * SCI access functions
 */
static void
scimci_putc(int c)
{

	CSR_SET_2(SH7709_SCPCR, SCPCR_CLK_OUT, SCPCR_CLK_MASK);
	SHREG_SCSCR = SCSCR_TE | SCSCR_SCK_OUT;
	while ((SHREG_SCSSR & SCSSR_TDRE) == 0)
		continue;
	CSR_CLR_2(SH7709_SCPCR, SCPCR_CLK_MASK);
	SHREG_SCTDR = (uint8_t)c;
	(void) SHREG_SCSSR;
	SHREG_SCSSR = 0;
}

static void
scimci_putc_sw(void)
{

	while ((SHREG_SCSSR & SCSSR_TEND) == 0)
		continue;

	CSR_SET_2(SH7709_SCPCR, SCPCR_CLK_IN, 0);
	SHREG_SCSCR |= SCSCR_SCK_IN;
	CSR_CLR_2(SH7709_SCPCR, SCPCR_CLK_MASK);
	SHREG_SCSMR = 0;
	SHREG_SCSCR = SCSCR_SCK_OUT;
	SHREG_SCSSR = 0;
	SHREG_SCSMR = SCSMR_CA;
}

static int
scimci_getc(void)
{
	int c;

	SHREG_SCSCR = SCSCR_RE | SCSCR_SCK_OUT;
	if (SHREG_SCSSR & SCSSR_ORER) {
		SHREG_SCSSR &= ~SCSSR_ORER;
		return -1;
	}
	CSR_CLR_2(SH7709_SCPCR, SCPCR_CLK_MASK);
	while ((SHREG_SCSSR & SCSSR_RDRF) == 0)
		continue;
	c = SHREG_SCRDR;
	(void) SHREG_SCSSR;
	SHREG_SCSSR = 0;

	return (uint8_t)c;
}

static void
scimci_getc_sw(void)
{

	SHREG_SCBRR = LOW_SPEED;
	while ((SHREG_SCSSR & SCSSR_RDRF) == 0)
		continue;
	(void) SHREG_SCRDR;

	CSR_SET_2(SH7709_SCPCR, SCPCR_CLK_IN, 0);
	SHREG_SCSCR |= SCSCR_SCK_IN;
	CSR_CLR_2(SH7709_SCPCR, SCPCR_CLK_MASK);
	SHREG_SCSMR = 0;
	SHREG_SCSCR = SCSCR_SCK_OUT;
	SHREG_SCSSR = 0;
	SHREG_SCSMR = SCSMR_CA;
}

/*
 * Reset the host controller.  Called during initialization, when
 * cards are removed, upon resume, and during error recovery.
 */
/*ARGSUSED*/
static int
scimci_host_reset(sdmmc_chipset_handle_t sch)
{

	return 0;
}

/*ARGSUSED*/
static uint32_t
scimci_host_ocr(sdmmc_chipset_handle_t sch)
{

	return MMC_OCR_3_2V_3_3V|MMC_OCR_3_3V_3_4V;
}

/*ARGSUSED*/
static int
scimci_host_maxblklen(sdmmc_chipset_handle_t sch)
{

	return 512;
}

/*ARGSUSED*/
static int
scimci_card_detect(sdmmc_chipset_handle_t sch)
{
	uint8_t reg;
	int s;

	s = splserial();
	CSR_SET_2(SH7709_PGCR, PGCR_EJECT, 0);
	reg = _reg_read_1(SH7709_PGDR);
	splx(s);

	return !(reg & PGDR_EJECT);
}

/*ARGSUSED*/
static int
scimci_write_protect(sdmmc_chipset_handle_t sch)
{

	return 0;	/* non-protect */
}

/*
 * Set or change SD bus voltage and enable or disable SD bus power.
 * Return zero on success.
 */
/*ARGSUSED*/
static int
scimci_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{

	if ((ocr & (MMC_OCR_3_2V_3_3V|MMC_OCR_3_3V_3_4V)) == 0)
		return 1;

	/*XXX???*/
	return 0;
}

/*
 * Set or change MMCLK frequency or disable the MMC clock.
 * Return zero on success.
 */
/*ARGSUSED*/
static int
scimci_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{

	return 0;
}

/*ARGSUSED*/
static int
scimci_bus_width(sdmmc_chipset_handle_t sch, int width)
{

	if (width != 1)
		return 1;
	return 0;
}

/*ARGSUSED*/
static void
scimci_spi_initialize(sdmmc_chipset_handle_t sch)
{
	int i, s;

	s = splserial();
	CSR_SET_1(SH7709_SCPDR, SCPDR_CS, 0);
	for (i = 0; i < 20; i++)
		scimci_putc(0xff);
	scimci_putc_sw();
	CSR_CLR_1(SH7709_SCPDR, SCPDR_CS);
	splx(s);
}

static void
scimci_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct scimci_softc *sc = (struct scimci_softc *)sch;
	uint16_t resp;
	int timo;
	int s;

	DPRINTF(1,("%s: start cmd %d arg=%#x data=%p dlen=%d flags=%#x "
	    "proc=%p \"%s\"\n", device_xname(sc->sc_dev),
	    cmd->c_opcode, cmd->c_arg, cmd->c_data, cmd->c_datalen,
	    cmd->c_flags, curproc, curproc ? curproc->p_comm : ""));

	s = splhigh();

	if (cmd->c_opcode == MMC_GO_IDLE_STATE)
		SHREG_SCBRR = LOW_SPEED;
	else
		SHREG_SCBRR = MID_SPEED;

	scimci_putc(0xff);
	scimci_putc(0x40 | (cmd->c_opcode & 0x3f));
	scimci_putc((cmd->c_arg >> 24) & 0xff);
	scimci_putc((cmd->c_arg >> 16) & 0xff);
	scimci_putc((cmd->c_arg >> 8) & 0xff);
	scimci_putc((cmd->c_arg >> 0) & 0xff);
	scimci_putc((cmd->c_opcode == MMC_GO_IDLE_STATE) ? 0x95 :
	    (cmd->c_opcode == SD_SEND_IF_COND) ? 0x87 : 0); /* CRC */
	scimci_putc(0xff);
	scimci_putc_sw();

	timo = MMC_TIME_OVER;
	while ((resp = scimci_getc()) & 0x80) {
		if(--timo == 0) {
			DPRINTF(1, ("%s: response timeout\n",
			    device_xname(sc->sc_dev)));
			scimci_getc_sw();
			cmd->c_error = ETIMEDOUT;
			goto out;
		}
	}
	if (ISSET(cmd->c_flags, SCF_RSP_SPI_S2)) {
		resp |= (uint16_t)scimci_getc() << 8;
	} else if (ISSET(cmd->c_flags, SCF_RSP_SPI_B4)) {
		cmd->c_resp[1] =  (uint32_t) scimci_getc() << 24;
		cmd->c_resp[1] |= (uint32_t) scimci_getc() << 16;
		cmd->c_resp[1] |= (uint32_t) scimci_getc() << 8;
		cmd->c_resp[1] |= (uint32_t) scimci_getc();
		DPRINTF(1, ("R3 resp: %#x\n", cmd->c_resp[1]));
	}
	scimci_getc_sw();

	cmd->c_resp[0] = resp;
	if (resp != 0 && resp != R1_SPI_IDLE) {
		DPRINTF(1, ("%s: response error: %#x\n",
		    device_xname(sc->sc_dev), resp));
		cmd->c_error = EIO;
		goto out;
	}
	DPRINTF(1, ("R1 resp: %#x\n", resp));

	if (cmd->c_datalen > 0) {
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			/* XXX: swap in this place? */
			if (cmd->c_opcode == MMC_SEND_CID ||
			    cmd->c_opcode == MMC_SEND_CSD) {
				sdmmc_response res;
				uint32_t *p = cmd->c_data;

				scimci_cmd_cfgread(sc, cmd);
				res[0] = be32toh(p[3]);
				res[1] = be32toh(p[2]);
				res[2] = be32toh(p[1]);
				res[3] = be32toh(p[0]);
				memcpy(p, &res, sizeof(res));
			} else {
				scimci_cmd_read(sc, cmd);
			}
		} else {
			scimci_cmd_write(sc, cmd);
		}
	}

out:
	SET(cmd->c_flags, SCF_ITSDONE);
	splx(s);

	DPRINTF(1,("%s: cmd %d done (flags=%#x error=%d)\n",
	  device_xname(sc->sc_dev), cmd->c_opcode, cmd->c_flags, cmd->c_error));
}

static void
scimci_cmd_cfgread(struct scimci_softc *sc, struct sdmmc_command *cmd)
{
	u_char *data = cmd->c_data;
	int timo;
	int c;
	int i;

	/* wait data token */
	for (timo = MMC_TIME_OVER; timo > 0; timo--) {
		c = scimci_getc();
		if (c < 0) {
			aprint_error_dev(sc->sc_dev, "cfg read i/o error\n");
			cmd->c_error = EIO;
			return;
		}
		if (c != 0xff)
			break;
	}
	if (timo == 0) {
		aprint_error_dev(sc->sc_dev, "cfg read timeout\n");
		cmd->c_error = ETIMEDOUT;
		return;
	}
	if (c != 0xfe) {
		aprint_error_dev(sc->sc_dev, "cfg read error (data=%#x)\n", c);
		cmd->c_error = EIO;
		return;
	}

	/* data read */
	SHREG_SCSCR = SCSCR_RE | SCSCR_SCK_OUT;
	data[0] = '\0'; /* XXXFIXME!!! */
	for (i = 1 /* XXXFIXME!!!*/ ; i < cmd->c_datalen; i++) {
		while ((SHREG_SCSSR & SCSSR_RDRF) == 0)
			continue;
		data[i] = SHREG_SCRDR;
		(void) SHREG_SCSSR;
		SHREG_SCSSR = 0;
	}

	SHREG_SCBRR = LOW_SPEED;
	(void) scimci_getc();
	(void) scimci_getc();
	(void) scimci_getc();
	scimci_getc_sw();

#ifdef SCIMCI_DEBUG
	sdmmc_dump_data(NULL, cmd->c_data, cmd->c_datalen);
#endif
}

static void
scimci_cmd_read(struct scimci_softc *sc, struct sdmmc_command *cmd)
{
	u_char *data = cmd->c_data;
	int timo;
	int c;
	int i;

	/* wait data token */
	for (timo = MMC_TIME_OVER; timo > 0; timo--) {
		c = scimci_getc();
		if (c < 0) {
			aprint_error_dev(sc->sc_dev, "read i/o error\n");
			cmd->c_error = EIO;
			return;
		}
		if (c != 0xff)
			break;
	}
	if (timo == 0) {
		aprint_error_dev(sc->sc_dev, "read timeout\n");
		cmd->c_error = ETIMEDOUT;
		return;
	}
	if (c != 0xfe) {
		aprint_error_dev(sc->sc_dev, "read error (data=%#x)\n", c);
		cmd->c_error = EIO;
		return;
	}

	/* data read */
	SHREG_SCBRR = MID_SPEED;
	SHREG_SCSCR = SCSCR_RE | SCSCR_SCK_OUT;
	for (i = 0; i < cmd->c_datalen; i++) {
		while ((SHREG_SCSSR & SCSSR_RDRF) == 0)
			continue;
		data[i] = SHREG_SCRDR;
		(void) SHREG_SCSSR;
		SHREG_SCSSR = 0;
	}

	SHREG_SCBRR = LOW_SPEED;
	(void) scimci_getc();
	(void) scimci_getc();
	(void) scimci_getc();
	scimci_getc_sw();

#ifdef SCIMCI_DEBUG
	sdmmc_dump_data(NULL, cmd->c_data, cmd->c_datalen);
#endif
}

static void
scimci_cmd_write(struct scimci_softc *sc, struct sdmmc_command *cmd)
{
	char *data = cmd->c_data;
	int timo;
	int c;
	int i;

	scimci_putc(0xff);
	scimci_putc(0xfe);

	/* data write */
	SHREG_SCBRR = MID_SPEED;
	SHREG_SCSCR = SCSCR_TE | SCSCR_SCK_OUT;
	for (i = 0; i < cmd->c_datalen; i++) {
		while ((SHREG_SCSSR & SCSSR_TDRE) == 0)
			continue;
		SHREG_SCTDR = data[i];
		(void) SHREG_SCSSR;
		SHREG_SCSSR = 0;
	}

	SHREG_SCBRR = LOW_SPEED;
	scimci_putc(0);
	scimci_putc(0);
	scimci_putc(0);
	scimci_putc_sw();

	for (timo = MMC_TIME_OVER; timo > 0; timo--) {
		c = scimci_getc();
		if (c < 0) {
			aprint_error_dev(sc->sc_dev, "write i/o error\n");
			cmd->c_error = EIO;
			scimci_getc_sw();
			return;
		}
		if (c == 0xff)
			break;
	}
	if (timo == 0) {
		aprint_error_dev(sc->sc_dev, "write timeout\n");
		cmd->c_error = ETIMEDOUT;
	}
	scimci_getc_sw();
}
