/* $NetBSD: pl181.c,v 1.7 2019/06/12 10:16:52 skrll Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pl181.c,v 1.7 2019/06/12 10:16:52 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmc_ioreg.h>

#include <dev/ic/pl181reg.h>
#include <dev/ic/pl181var.h>

/*
 * Data length register is 16 bits for a maximum of 65535 bytes. Round
 * maximum transfer size down to the nearest sector.
 */
#define	PLMMC_MAXXFER	rounddown(65535, SDMMC_SECTOR_SIZE)

/*
 * PL181 FIFO is 16 words deep (64 bytes)
 */
#define	PL181_FIFO_DEPTH	64

/*
 * Data transfer IRQ status bits
 */
#define	PLMMC_INT_DATA_MASK						\
	(MMCI_INT_DATA_TIMEOUT|MMCI_INT_DATA_CRC_FAIL|			\
	 MMCI_INT_TX_FIFO_EMPTY|MMCI_INT_TX_FIFO_HALF_EMPTY|		\
	 MMCI_INT_RX_FIFO_FULL|MMCI_INT_RX_FIFO_HALF_FULL|		\
	 MMCI_INT_DATA_END|MMCI_INT_DATA_BLOCK_END)
#define	PLMMC_INT_CMD_MASK						\
	(MMCI_INT_CMD_TIMEOUT|MMCI_INT_CMD_RESP_END)

static int	plmmc_host_reset(sdmmc_chipset_handle_t);
static uint32_t	plmmc_host_ocr(sdmmc_chipset_handle_t);
static int	plmmc_host_maxblklen(sdmmc_chipset_handle_t);
static int	plmmc_card_detect(sdmmc_chipset_handle_t);
static int	plmmc_write_protect(sdmmc_chipset_handle_t);
static int	plmmc_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	plmmc_bus_clock(sdmmc_chipset_handle_t, int);
static int	plmmc_bus_width(sdmmc_chipset_handle_t, int);
static int	plmmc_bus_rod(sdmmc_chipset_handle_t, int);
static void	plmmc_exec_command(sdmmc_chipset_handle_t,
				     struct sdmmc_command *);
static void	plmmc_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	plmmc_card_intr_ack(sdmmc_chipset_handle_t);

static int	plmmc_wait_cmd(struct plmmc_softc *);
static int	plmmc_pio_transfer(struct plmmc_softc *,
				     struct sdmmc_command *, int);

static struct sdmmc_chip_functions plmmc_chip_functions = {
	.host_reset = plmmc_host_reset,
	.host_ocr = plmmc_host_ocr,
	.host_maxblklen = plmmc_host_maxblklen,
	.card_detect = plmmc_card_detect,
	.write_protect = plmmc_write_protect,
	.bus_power = plmmc_bus_power,
	.bus_clock = plmmc_bus_clock,
	.bus_width = plmmc_bus_width,
	.bus_rod = plmmc_bus_rod,
	.exec_command = plmmc_exec_command,
	.card_enable_intr = plmmc_card_enable_intr,
	.card_intr_ack = plmmc_card_intr_ack,
};

#define MMCI_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define	MMCI_WRITE_MULTI(sc, reg, datap, cnt) \
	bus_space_write_multi_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (datap), (cnt))
#define MMCI_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	MMCI_READ_MULTI(sc, reg, datap, cnt) \
	bus_space_read_multi_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (datap), (cnt))

void
plmmc_init(struct plmmc_softc *sc)
{
	struct sdmmcbus_attach_args saa;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_intr_cv, "plmmcirq");

#ifdef PLMMC_DEBUG
	device_printf(sc->sc_dev, "PeriphID %#x %#x %#x %#x\n",
	    MMCI_READ(sc, MMCI_PERIPH_ID0_REG),
	    MMCI_READ(sc, MMCI_PERIPH_ID1_REG),
	    MMCI_READ(sc, MMCI_PERIPH_ID2_REG),
	    MMCI_READ(sc, MMCI_PERIPH_ID3_REG));
	device_printf(sc->sc_dev, "PCellID %#x %#x %#x %#x\n",
	    MMCI_READ(sc, MMCI_PCELL_ID0_REG),
	    MMCI_READ(sc, MMCI_PCELL_ID1_REG),
	    MMCI_READ(sc, MMCI_PCELL_ID2_REG),
	    MMCI_READ(sc, MMCI_PCELL_ID3_REG));
#endif

	plmmc_bus_clock(sc, 400);
	MMCI_WRITE(sc, MMCI_POWER_REG, 0);
	delay(10000);
	MMCI_WRITE(sc, MMCI_POWER_REG, MMCI_POWER_CTRL_POWERUP);
	delay(10000);
	MMCI_WRITE(sc, MMCI_POWER_REG, MMCI_POWER_CTRL_POWERON);
	plmmc_host_reset(sc);

	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &plmmc_chip_functions;
	saa.saa_sch = sc;
	saa.saa_clkmin = 400;
	saa.saa_clkmax = sc->sc_max_freq > 0 ?
	    sc->sc_max_freq / 1000 : sc->sc_clock_freq / 1000;
	saa.saa_caps = SMC_CAPS_4BIT_MODE;

	sc->sc_sdmmc_dev = config_found(sc->sc_dev, &saa, NULL);
}

static int
plmmc_intr_xfer(struct plmmc_softc *sc, struct sdmmc_command *cmd)
{
	uint32_t len;

	if (cmd == NULL) {
		device_printf(sc->sc_dev, "TX/RX interrupt with no active transfer\n");
		return EINVAL;
	}

	if (cmd->c_buf == NULL) {
		return EINVAL;
	}

	const uint32_t fifo_cnt =
	    __SHIFTOUT(MMCI_READ(sc, MMCI_FIFO_CNT_REG), MMCI_FIFO_CNT) * 4;
	if (fifo_cnt > sc->sc_fifo_resid) {
		device_printf(sc->sc_dev, "FIFO counter is out of sync with active transfer\n");
		return EIO;
	}

	if (cmd->c_flags & SCF_CMD_READ)
		len = sc->sc_fifo_resid - fifo_cnt;
	else
		len = uimin(sc->sc_fifo_resid, PL181_FIFO_DEPTH);

	if (len == 0)
		return 0;

	if (cmd->c_flags & SCF_CMD_READ)
		MMCI_READ_MULTI(sc, MMCI_FIFO_REG, (uint32_t *)cmd->c_buf, len / 4);
	else
		MMCI_WRITE_MULTI(sc, MMCI_FIFO_REG, (uint32_t *)cmd->c_buf, len / 4);

	sc->sc_fifo_resid -= len;
	cmd->c_resid -= len;
	cmd->c_buf += len;

	return 0;
}

int
plmmc_intr(void *priv)
{
	struct plmmc_softc *sc = priv;
	uint32_t status, mask;
	int retry = 100000;

	mutex_enter(&sc->sc_lock);

	while (--retry > 0) {
		status = MMCI_READ(sc, MMCI_STATUS_REG);
#ifdef PLMMC_DEBUG
		printf("%s: MMCI_STATUS_REG = %#x\n", __func__, status);
#endif
		if ((status & sc->sc_status_mask) == 0)
			break;
		MMCI_WRITE(sc, MMCI_CLEAR_REG, status);
		sc->sc_intr_status |= status;

		if (status & MMCI_INT_CMD_TIMEOUT)
			break;

		if (status & (MMCI_INT_DATA_TIMEOUT|MMCI_INT_DATA_CRC_FAIL)) {
			device_printf(sc->sc_dev,
			    "data xfer error, status %08x\n", status);
			break;
		}

		if (status & (MMCI_INT_TX_FIFO_EMPTY|MMCI_INT_TX_FIFO_HALF_EMPTY|
			      MMCI_INT_RX_FIFO_FULL|MMCI_INT_RX_FIFO_HALF_FULL|
			      MMCI_INT_DATA_END|MMCI_INT_DATA_BLOCK_END)) {

			/* Data transfer in progress */
			if (plmmc_intr_xfer(sc, sc->sc_cmd) == 0 &&
			    sc->sc_fifo_resid == 0) {
				/* Disable data IRQs */
				mask = MMCI_READ(sc, MMCI_MASK0_REG);
				mask &= ~PLMMC_INT_DATA_MASK;
				MMCI_WRITE(sc, MMCI_MASK0_REG, mask);
				/* Ignore data status bits after transfer */
				sc->sc_status_mask &= ~PLMMC_INT_DATA_MASK;
			}
		}

		if (status & MMCI_INT_CMD_RESP_END)
			cv_broadcast(&sc->sc_intr_cv);
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "intr handler stuck, fifo resid %d, status %08x\n",
		    sc->sc_fifo_resid, MMCI_READ(sc, MMCI_STATUS_REG));
	}

	cv_broadcast(&sc->sc_intr_cv);
	mutex_exit(&sc->sc_lock);

	return 1;
}

static int
plmmc_wait_cmd(struct plmmc_softc *sc)
{
	int error = 0;

	KASSERT(mutex_owned(&sc->sc_lock));

	while (error == 0) {
		if (sc->sc_intr_status & MMCI_INT_CMD_TIMEOUT) {
			error = ETIMEDOUT;
			break;
		} else if (sc->sc_intr_status & MMCI_INT_CMD_RESP_END) {
			break;
		}

		error = cv_timedwait(&sc->sc_intr_cv, &sc->sc_lock, hz * 2);
		if (error != 0)
			break;
	}

	return error;
}

static int
plmmc_pio_transfer(struct plmmc_softc *sc, struct sdmmc_command *cmd,
    int xferlen)
{
	int error = 0;

	while (sc->sc_fifo_resid > 0 && error == 0) {
		error = cv_timedwait(&sc->sc_intr_cv,
		    &sc->sc_lock, hz * 5);
		if (error != 0)
			break;

		if (sc->sc_intr_status & MMCI_INT_DATA_TIMEOUT)
			error = ETIMEDOUT;
		else if (sc->sc_intr_status & MMCI_INT_DATA_CRC_FAIL)
			error = EIO;
	}

	return error;
}

static int
plmmc_host_reset(sdmmc_chipset_handle_t sch)
{
	struct plmmc_softc *sc = sch;

	MMCI_WRITE(sc, MMCI_MASK0_REG, 0);
	MMCI_WRITE(sc, MMCI_MASK1_REG, 0);
	MMCI_WRITE(sc, MMCI_CLEAR_REG, 0xffffffff);

	return 0;
}

static uint32_t
plmmc_host_ocr(sdmmc_chipset_handle_t sch)
{
	return MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V;
}

static int
plmmc_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	return 2048;
}

static int
plmmc_card_detect(sdmmc_chipset_handle_t sch)
{
	return 1;
}

static int
plmmc_write_protect(sdmmc_chipset_handle_t sch)
{
	return 0;
}

static int
plmmc_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	return 0;
}

static int
plmmc_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct plmmc_softc *sc = sch;
	u_int pll_freq, clk_div;
	uint32_t clock;

	clock = MMCI_CLOCK_PWRSAVE;
	if (freq) {
		pll_freq = sc->sc_clock_freq / 1000;
		clk_div = (howmany(pll_freq, freq) >> 1) - 1;
		clock |= __SHIFTIN(clk_div, MMCI_CLOCK_CLKDIV);
		clock |= MMCI_CLOCK_ENABLE;
	}
	MMCI_WRITE(sc, MMCI_CLOCK_REG, clock);

	return 0;
}

static int
plmmc_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	return 0;
}

static int
plmmc_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	struct plmmc_softc *sc = sch;
	uint32_t power;


	power = MMCI_READ(sc, MMCI_POWER_REG);
	if (on) {
		power |= MMCI_POWER_ROD;
	} else {
		power &= ~MMCI_POWER_ROD;
	}
	MMCI_WRITE(sc, MMCI_POWER_REG, power);

	return 0;
}

static void
plmmc_do_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct plmmc_softc *sc = sch;
	uint32_t cmdval = MMCI_COMMAND_ENABLE;

	KASSERT(mutex_owned(&sc->sc_lock));

	const int xferlen = uimin(cmd->c_resid, PLMMC_MAXXFER);

	sc->sc_cmd = cmd;
	sc->sc_fifo_resid = xferlen;
	sc->sc_status_mask = ~0U;
	sc->sc_intr_status = 0;

#ifdef PLMMC_DEBUG
	device_printf(sc->sc_dev,
	    "opcode %d flags %#x datalen %d resid %d xferlen %d\n",
	    cmd->c_opcode, cmd->c_flags, cmd->c_datalen, cmd->c_resid, xferlen);
#endif

	MMCI_WRITE(sc, MMCI_COMMAND_REG, 0);
	MMCI_WRITE(sc, MMCI_MASK0_REG, 0);
	MMCI_WRITE(sc, MMCI_CLEAR_REG, 0xffffffff);
	MMCI_WRITE(sc, MMCI_MASK0_REG, PLMMC_INT_DATA_MASK | PLMMC_INT_CMD_MASK);

	if (cmd->c_flags & SCF_RSP_PRESENT)
		cmdval |= MMCI_COMMAND_RESPONSE;
	if (cmd->c_flags & SCF_RSP_136)
		cmdval |= MMCI_COMMAND_LONGRSP;

	uint32_t arg = cmd->c_arg;

	if (xferlen > 0) {
		unsigned int nblks = xferlen / cmd->c_blklen;
		if (nblks == 0 || (xferlen % cmd->c_blklen) != 0)
			++nblks;

		const uint32_t dir = (cmd->c_flags & SCF_CMD_READ) ? 1 : 0;
		const uint32_t blksize = ffs(cmd->c_blklen) - 1;

		MMCI_WRITE(sc, MMCI_DATA_TIMER_REG, 0xffffffff);
		MMCI_WRITE(sc, MMCI_DATA_LENGTH_REG, nblks * cmd->c_blklen);
		MMCI_WRITE(sc, MMCI_DATA_CTRL_REG,
		    __SHIFTIN(dir, MMCI_DATA_CTRL_DIRECTION) |
		    __SHIFTIN(blksize, MMCI_DATA_CTRL_BLOCKSIZE) |
		    MMCI_DATA_CTRL_ENABLE);

		/* Adjust blkno if necessary */
		u_int blkoff =
		    (cmd->c_datalen - cmd->c_resid) / SDMMC_SECTOR_SIZE;
		if (!ISSET(cmd->c_flags, SCF_XFER_SDHC))
			blkoff <<= SDMMC_SECTOR_SIZE_SB;
		arg += blkoff;
	}

	MMCI_WRITE(sc, MMCI_ARGUMENT_REG, arg);
	MMCI_WRITE(sc, MMCI_COMMAND_REG, cmdval | cmd->c_opcode);

	if (xferlen > 0) {
		cmd->c_error = plmmc_pio_transfer(sc, cmd, xferlen);
		if (cmd->c_error) {
#ifdef PLMMC_DEBUG
			device_printf(sc->sc_dev,
			    "MMCI_STATUS_REG = %08x\n", MMCI_READ(sc, MMCI_STATUS_REG));
#endif
			device_printf(sc->sc_dev,
			    "error (%d) waiting for xfer\n", cmd->c_error);
			goto done;
		}
	}

	if ((cmd->c_flags & SCF_RSP_PRESENT) && cmd->c_resid == 0) {
		cmd->c_error = plmmc_wait_cmd(sc);
		if (cmd->c_error) {
#ifdef PLMMC_DEBUG
			device_printf(sc->sc_dev,
			    "error (%d) waiting for resp\n", cmd->c_error);
#endif
			goto done;
		}

		if (cmd->c_flags & SCF_RSP_136) {
			cmd->c_resp[3] = MMCI_READ(sc, MMCI_RESP0_REG);
			cmd->c_resp[2] = MMCI_READ(sc, MMCI_RESP1_REG);
			cmd->c_resp[1] = MMCI_READ(sc, MMCI_RESP2_REG);
			cmd->c_resp[0] = MMCI_READ(sc, MMCI_RESP3_REG);
			if (cmd->c_flags & SCF_RSP_CRC) {
				cmd->c_resp[0] = (cmd->c_resp[0] >> 8) |
				    (cmd->c_resp[1] << 24);
				cmd->c_resp[1] = (cmd->c_resp[1] >> 8) |
				    (cmd->c_resp[2] << 24);
				cmd->c_resp[2] = (cmd->c_resp[2] >> 8) |
				    (cmd->c_resp[3] << 24);
				cmd->c_resp[3] = (cmd->c_resp[3] >> 8);
			}
		} else {
			cmd->c_resp[0] = MMCI_READ(sc, MMCI_RESP0_REG);
		}
	}

done:
	sc->sc_cmd = NULL;

	MMCI_WRITE(sc, MMCI_COMMAND_REG, 0);
	MMCI_WRITE(sc, MMCI_MASK0_REG, 0);
	MMCI_WRITE(sc, MMCI_CLEAR_REG, 0xffffffff);
	MMCI_WRITE(sc, MMCI_DATA_CNT_REG, 0);

#ifdef PLMMC_DEBUG
	device_printf(sc->sc_dev, "status = %#x\n", sc->sc_intr_status);
#endif
}

static void
plmmc_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct plmmc_softc *sc = sch;

#ifdef PLMMC_DEBUG
	device_printf(sc->sc_dev, "opcode %d flags %#x data %p datalen %d\n",
	    cmd->c_opcode, cmd->c_flags, cmd->c_data, cmd->c_datalen);
#endif

	mutex_enter(&sc->sc_lock);
	cmd->c_resid = cmd->c_datalen;
	cmd->c_buf = cmd->c_data;
	do {
		plmmc_do_command(sch, cmd);

		if (cmd->c_resid > 0 && cmd->c_error == 0) {
			/*
			 * Multi block transfer and there is still data
			 * remaining. Send a stop cmd between transfers.
			 */
			struct sdmmc_command stop_cmd;
			memset(&stop_cmd, 0, sizeof(stop_cmd));
			stop_cmd.c_opcode = MMC_STOP_TRANSMISSION;
			stop_cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1B |
			    SCF_RSP_SPI_R1B;
			plmmc_do_command(sch, &stop_cmd);
		}
	} while (cmd->c_resid > 0 && cmd->c_error == 0);
	cmd->c_flags |= SCF_ITSDONE;
	mutex_exit(&sc->sc_lock);
}

static void
plmmc_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
}

static void
plmmc_card_intr_ack(sdmmc_chipset_handle_t sch)
{
}
