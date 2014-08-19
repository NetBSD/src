/*	$NetBSD: ssumci.c,v 1.2.6.1 2014/08/20 00:02:59 tls Exp $	*/

/*-
 * Copyright (C) 2010 NONAKA Kimihiro <nonaka@netbsd.org>
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

/*
 * driver to access MMC/SD card
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ssumci.c,v 1.2.6.1 2014/08/20 00:02:59 tls Exp $");

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

#include <machine/autoconf.h>

#include <evbsh3/t_sh7706lan/t_sh7706lanvar.h>

#ifdef SSUMCI_DEBUG
int ssumci_debug = 1;
#define DPRINTF(n,s)	do { if ((n) <= ssumci_debug) printf s; } while (0)
#else
#define DPRINTF(n,s)	do {} while (0)
#endif

static int	ssumci_host_reset(sdmmc_chipset_handle_t);
static uint32_t	ssumci_host_ocr(sdmmc_chipset_handle_t);
static int	ssumci_host_maxblklen(sdmmc_chipset_handle_t);
static int	ssumci_card_detect(sdmmc_chipset_handle_t);
static int	ssumci_write_protect(sdmmc_chipset_handle_t);
static int	ssumci_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	ssumci_bus_clock(sdmmc_chipset_handle_t, int);
static int	ssumci_bus_width(sdmmc_chipset_handle_t, int);
static void	ssumci_exec_command(sdmmc_chipset_handle_t,
		    struct sdmmc_command *);

static struct sdmmc_chip_functions ssumci_chip_functions = {
	/* host controller reset */
	.host_reset		= ssumci_host_reset,

	/* host controller capabilities */
	.host_ocr		= ssumci_host_ocr,
	.host_maxblklen		= ssumci_host_maxblklen,

	/* card detection */
	.card_detect		= ssumci_card_detect,

	/* write protect */
	.write_protect		= ssumci_write_protect,

	/* bus power, clock frequency, width */
	.bus_power		= ssumci_bus_power,
	.bus_clock		= ssumci_bus_clock,
	.bus_width		= ssumci_bus_width,

	/* command execution */
	.exec_command		= ssumci_exec_command,

	/* card interrupt */
	.card_enable_intr	= NULL,
	.card_intr_ack		= NULL,
};

static void	ssumci_spi_initialize(sdmmc_chipset_handle_t);

static struct sdmmc_spi_chip_functions ssumci_spi_chip_functions = {
	.initialize		= ssumci_spi_initialize,
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
#define	SCPCR_CS_MASK	0x000C
#define	SCPCR_CS_OUT	0x0004
#define	SCPDR_CS	0x02
#define	SCPCR_EJECT	0x00C0
#define	SCPDR_EJECT	0x08

#define MMC_TIME_OVER	20000

struct ssumci_softc {
	device_t sc_dev;
	device_t sc_sdmmc;
};

static int ssumci_match(device_t, cfdata_t, void *);
static void ssumci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ssumci, sizeof(struct ssumci_softc),
    ssumci_match, ssumci_attach, NULL, NULL);

static void ssumci_cmd_cfgread(struct ssumci_softc *, struct sdmmc_command *);
static void ssumci_cmd_read(struct ssumci_softc *, struct sdmmc_command *);
static void ssumci_cmd_write(struct ssumci_softc *, struct sdmmc_command *);

#define	SSUMCI_SPIDR	0xb0008000
#define	SSUMCI_SPISR	0xb0008002
#define	SSUMCI_SPIBR	0xb0008004

static inline void
ssumci_wait(void)
{

	while (_reg_read_1(SSUMCI_SPISR) == 0x00)
		continue;
}

static inline uint8_t
ssumci_getc(void)
{

	ssumci_wait();
	return _reg_read_1(SSUMCI_SPIBR);
}

static inline void
ssumci_putc(uint8_t v)
{

	_reg_write_1(SSUMCI_SPIDR, v);
	ssumci_wait();
}

/*ARGSUSED*/
static int
ssumci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = (struct mainbus_attach_args *)aux;

	if (strcmp(maa->ma_name, "ssumci") != 0)
		return 0;
	if (!IS_SH7706LSR)
		return 0;
	return 1;
}

/*ARGSUSED*/
static void
ssumci_attach(device_t parent, device_t self, void *aux)
{
	struct ssumci_softc *sc = device_private(self);
	struct sdmmcbus_attach_args saa;

	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal(": SPI MMC controller\n");

	/* setup */
	CSR_SET_2(SH7709_SCPCR, SCPCR_CS_OUT, SCPCR_CS_MASK);

	/*
	 * Attach the generic SD/MMC bus driver.  (The bus driver must
	 * not invoke any chipset functions before it is attached.)
	 */
	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &ssumci_chip_functions;
	saa.saa_spi_sct = &ssumci_spi_chip_functions;
	saa.saa_sch = sc;
	saa.saa_clkmin = 400;
	saa.saa_clkmax = 400;
	saa.saa_caps = SMC_CAPS_SPI_MODE
	               | SMC_CAPS_SINGLE_ONLY
	               | SMC_CAPS_POLL_CARD_DET;

	sc->sc_sdmmc = config_found(sc->sc_dev, &saa, NULL);
	if (sc->sc_sdmmc == NULL)
		aprint_error_dev(sc->sc_dev, "couldn't attach bus\n");
}

/*
 * Reset the host controller.  Called during initialization, when
 * cards are removed, upon resume, and during error recovery.
 */
/*ARGSUSED*/
static int
ssumci_host_reset(sdmmc_chipset_handle_t sch)
{

	return 0;
}

/*ARGSUSED*/
static uint32_t
ssumci_host_ocr(sdmmc_chipset_handle_t sch)
{

	return MMC_OCR_3_2V_3_3V|MMC_OCR_3_3V_3_4V;
}

/*ARGSUSED*/
static int
ssumci_host_maxblklen(sdmmc_chipset_handle_t sch)
{

	return 512;
}

/*ARGSUSED*/
static int
ssumci_card_detect(sdmmc_chipset_handle_t sch)
{
	uint8_t reg;
	int s;

	s = splsdmmc();
	CSR_SET_2(SH7709_SCPCR, SCPCR_EJECT, 0);
	reg = _reg_read_1(SH7709_SCPDR);
	splx(s);

	return !(reg & SCPDR_EJECT);
}

/*ARGSUSED*/
static int
ssumci_write_protect(sdmmc_chipset_handle_t sch)
{

	return 0;	/* non-protect */
}

/*
 * Set or change SD bus voltage and enable or disable SD bus power.
 * Return zero on success.
 */
/*ARGSUSED*/
static int
ssumci_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
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
ssumci_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{

	return 0;
}

/*ARGSUSED*/
static int
ssumci_bus_width(sdmmc_chipset_handle_t sch, int width)
{

	if (width != 1)
		return 1;
	return 0;
}

/*ARGSUSED*/
static void
ssumci_spi_initialize(sdmmc_chipset_handle_t sch)
{
	int i, s;

	s = splsdmmc();
	CSR_SET_1(SH7709_SCPDR, SCPDR_CS, 0);
	for (i = 0; i < 10; i++) {
		ssumci_putc(0xff);
	}
	CSR_CLR_1(SH7709_SCPDR, SCPDR_CS);
	splx(s);
}

static void
ssumci_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct ssumci_softc *sc = (struct ssumci_softc *)sch;
	uint16_t resp;
	int timo;
	int s;

	DPRINTF(1,("%s: start cmd %d arg=%#x data=%p dlen=%d flags=%#x\n",
	    device_xname(sc->sc_dev), cmd->c_opcode, cmd->c_arg, cmd->c_data,
	    cmd->c_datalen, cmd->c_flags));

	s = splsdmmc();

	ssumci_putc(0xff);
	ssumci_putc(0x40 | (cmd->c_opcode & 0x3f));
	ssumci_putc((cmd->c_arg >> 24) & 0xff);
	ssumci_putc((cmd->c_arg >> 16) & 0xff);
	ssumci_putc((cmd->c_arg >> 8) & 0xff);
	ssumci_putc((cmd->c_arg >> 0) & 0xff);
	ssumci_putc((cmd->c_opcode == MMC_GO_IDLE_STATE) ? 0x95 :
	    (cmd->c_opcode == SD_SEND_IF_COND) ? 0x87 : 0); /* CRC */

	for (timo = MMC_TIME_OVER; timo > 0; timo--) {
		resp = ssumci_getc();
		if (!(resp & 0x80) && timo <= (MMC_TIME_OVER - 2))
			break;
	}
	if (timo == 0) {
		DPRINTF(1,(sc->sc_dev, "response timeout\n"));
		cmd->c_error = ETIMEDOUT;
		goto out;
	}

	if (ISSET(cmd->c_flags, SCF_RSP_SPI_S2)) {
		resp |= (uint16_t) ssumci_getc() << 8;
	} else if (ISSET(cmd->c_flags, SCF_RSP_SPI_B4)) {
		cmd->c_resp[1] =  (uint32_t) ssumci_getc() << 24;
		cmd->c_resp[1] |= (uint32_t) ssumci_getc() << 16;
		cmd->c_resp[1] |= (uint32_t) ssumci_getc() << 8;
		cmd->c_resp[1] |= (uint32_t) ssumci_getc();
		DPRINTF(1, ("R3 resp: %#x\n", cmd->c_resp[1]));
	}
	cmd->c_resp[0] = resp;
	if (resp != 0 && resp != R1_SPI_IDLE) {
		DPRINTF(1,("response error: %#x\n", resp));
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

				ssumci_cmd_cfgread(sc, cmd);
				res[0] = be32toh(p[3]);
				res[1] = be32toh(p[2]);
				res[2] = be32toh(p[1]);
				res[3] = be32toh(p[0]);
				memcpy(p, &res, sizeof(res));
			} else {
				ssumci_cmd_read(sc, cmd);
			}
		} else {
			ssumci_cmd_write(sc, cmd);
		}
	} else {
		ssumci_wait();
	}

out:
	SET(cmd->c_flags, SCF_ITSDONE);
	splx(s);

	DPRINTF(1,("%s: cmd %d done (flags=%#x error=%d)\n",
	  device_xname(sc->sc_dev), cmd->c_opcode, cmd->c_flags, cmd->c_error));
}

static void
ssumci_cmd_cfgread(struct ssumci_softc *sc, struct sdmmc_command *cmd)
{
	u_char *data = cmd->c_data;
	int timo;
	int c;
	int i;

	/* wait data token */
	for (timo = MMC_TIME_OVER; timo > 0; timo--) {
		c = ssumci_getc();
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
	data[0] = '\0'; /* XXXFIXME!!! */
	for (i = 1 /* XXXFIXME!!!*/ ; i < cmd->c_datalen; i++) {
		data[i] = ssumci_getc();
	}

	(void) ssumci_getc();
	(void) ssumci_getc();
	ssumci_wait();

#ifdef SSUMCI_DEBUG
	sdmmc_dump_data(NULL, cmd->c_data, cmd->c_datalen);
#endif
}

static void
ssumci_cmd_read(struct ssumci_softc *sc, struct sdmmc_command *cmd)
{
	u_char *data = cmd->c_data;
	int timo;
	int c;
	int i;

	/* wait data token */
	for (timo = MMC_TIME_OVER; timo > 0; timo--) {
		c = ssumci_getc();
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
	for (i = 0; i < cmd->c_datalen; i++) {
		data[i] = ssumci_getc();
	}

	/* ignore CRC */
	(void) ssumci_getc();
	(void) ssumci_getc();
	ssumci_wait();

#ifdef SSUMCI_DEBUG
	sdmmc_dump_data(NULL, cmd->c_data, cmd->c_datalen);
#endif
}

static void
ssumci_cmd_write(struct ssumci_softc *sc, struct sdmmc_command *cmd)
{
	u_char *data = cmd->c_data;
	int timo;
	int c;
	int i;

	ssumci_wait();
	ssumci_putc(0xfe);

	/* data write */
	for (i = 0; i < cmd->c_datalen; i++) {
		ssumci_putc(data[i]);
	}

	/* dummy CRC */
	ssumci_putc(0);
	ssumci_putc(0);
	ssumci_putc(0xff);
	if ((_reg_read_1(SSUMCI_SPIDR) & 0x0f) != 5) {
		aprint_error_dev(sc->sc_dev, "write error\n");
		cmd->c_error = EIO;
		return;
	}

	for (timo = 0x7fffffff; timo > 0; timo--) {
		ssumci_putc(0xff);
		c = _reg_read_1(SSUMCI_SPIDR);
		if (c == 0xff)
			break;
	}
	if (timo == 0) {
		aprint_error_dev(sc->sc_dev, "write timeout\n");
		cmd->c_error = ETIMEDOUT;
	}
}
