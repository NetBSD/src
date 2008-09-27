/* $NetBSD: spdmem.c,v 1.9 2008/09/27 06:58:08 pgoyette Exp $ */

/*
 * Copyright (c) 2007 Nicolas Joly
 * Copyright (c) 2007 Paul Goyette
 * Copyright (c) 2007 Tobias Nygren
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
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
 * Serial Presence Detect (SPD) memory identification
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spdmem.c,v 1.9 2008/09/27 06:58:08 pgoyette Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/sysctl.h>
#include <machine/bswap.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/spdmemreg.h>
#include <dev/i2c/spdmemvar.h>

static int spdmem_match(device_t, cfdata_t, void *);
static void spdmem_attach(device_t, device_t, void *);
SYSCTL_SETUP_PROTO(sysctl_spdmem_setup);

static uint8_t spdmem_read(struct spdmem_softc *, uint8_t);

CFATTACH_DECL_NEW(spdmem, sizeof(struct spdmem_softc),
    spdmem_match, spdmem_attach, NULL, NULL);

#define IS_RAMBUS_TYPE (s->sm_len < 4)

static const char* spdmem_basic_types[] = {
	"unknown",
	"FPM",
	"EDO",
	"Pipelined Nibble",
	"SDRAM",
	"ROM",
	"DDR SGRAM",
	"DDR SDRAM",
	"DDR2 SDRAM",
	"DDR2 SDRAM FB",
	"DDR2 SDRAM FB Probe"
};

static const char* spdmem_superset_types[] = {
	"unknown",
	"ESDRAM",
	"DDR ESDRAM",
	"PEM EDO",
	"PEM SDRAM"
};

static const char* spdmem_voltage_types[] = {
	"TTL (5V tolerant)",
	"LvTTL (not 5V tolerant)",
	"HSTL 1.5V",
	"SSTL 3.3V",
	"SSTL 2.5V",
	"SSTL 1.8V"
};

static const char* spdmem_refresh_types[] = {
	"15.625us",
	"3.9us",
	"7.8us",
	"31.3us",
	"62.5us",
	"125us"
};

static const char* spdmem_parity_types[] = {
	"no parity or ECC",
	"data parity",
	"data ECC",
	"data parity and ECC",
	"cmd/addr parity",
	"cmd/addr/data parity",
	"cmd/addr parity, data ECC",
	"cmd/addr/data parity, data ECC"
};

/* Cycle time fractional values for DDR2 SDRAM */
static const uint8_t spdmem_cycle_frac[] = {
	0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 25, 33, 67, 75, 99, 99
};

/* sysctl stuff */
static int hw_node = CTL_EOL;

/* CRC functions used for certain memory types */

static uint16_t spdcrc16 (struct spdmem_softc *sc, int count)
{
	uint16_t crc;
	int i, j;
	uint8_t val;
	crc = 0;
	for (j = 0; j <= count; j++) {
		val = spdmem_read(sc, j);
		crc = crc ^ val << 8;
		for (i = 0; i < 8; ++i)
			if (crc & 0x8000)
				crc = crc << 1 ^ 0x1021;
			else
				crc = crc << 1;
	}
	return (crc & 0xFFFF);
}
static int
spdmem_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	struct spdmem_softc sc;
	int cksum = 0;
	uint8_t i, val, spd_type;
	int spd_len, spd_crc_cover;
	uint16_t crc_calc, crc_spd;

	if ((ia->ia_addr & SPDMEM_ADDRMASK) != SPDMEM_ADDR)
		return 0;

	sc.sc_tag = ia->ia_tag;
	sc.sc_addr = ia->ia_addr;

	spd_type = spdmem_read(&sc, 2);

	/* For older memory types, validate the checksum over 1st 63 bytes */
	if (spd_type <= SPDMEM_MEMTYPE_DDR2SDRAM) {
		for (i = 0; i < 63; i++)
			cksum += spdmem_read(&sc, i);

		val = spdmem_read(&sc, 63);

		if (cksum == 0 || (cksum & 0xff) != val) {
			aprint_debug("spd addr 0x%2x: ", sc.sc_addr);
			aprint_debug("spd checksum failed, calc = 0x%02x, "
				     "spd = 0x%02x\n", cksum, val);
			return 0;
		} else
			return 1;
	}

	/* For FBDIMM and newer, verify the CRC */
	else {
		spd_len = spdmem_read(&sc, 0);
		if (spd_len && SPDMEM_SPDCRC_116)
			spd_crc_cover = 116;
		else
			spd_crc_cover = 125;
		switch (spd_len & SPDMEM_SPDLEN_MASK) {
		case SPDMEM_SPDLEN_128:
			spd_len = 128;
			break;
		case SPDMEM_SPDLEN_176:
			spd_len = 176;
			break;
		case SPDMEM_SPDLEN_256:
			spd_len = 256;
			break;
		default:
			return 0;
		}
		if (spd_crc_cover > spd_len)
			return 0;
		crc_calc = spdcrc16(&sc, spd_crc_cover);
		crc_spd = spdmem_read(&sc, 127) << 8;
		crc_spd |= spdmem_read(&sc, 126);
		if (crc_calc != crc_spd) {
			aprint_debug("spd addr 0x%2x: ", sc.sc_addr);
			aprint_debug("crc16 failed, covers %d bytes, "
				     "calc = 0x%04x, spd = 0x%04x\n",
				     spd_crc_cover, crc_calc, crc_spd);
			return 0;
		}
		return 1;
	}
}

static void
spdmem_attach(device_t parent, device_t self, void *aux)
{
	struct spdmem_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	struct spdmem *s = &(sc->sc_spd_data);
	const char *type;
	const char *voltage;
	const char *refresh;
	const char *ddr_type_string = NULL;
	const char *rambus_rev = "Reserved";
	int num_banks = 0;
	int per_chip = 0;
	int dimm_size, cycle_time, d_clk, p_clk, bits;
	int i;
	unsigned int spd_len, spd_size;
	const struct sysctlnode *node = NULL;

	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/*
	 * FBDIMM (and probably all newer) has a different
	 * encoding of the SPD EEPROM used/total sizes
	 */
	s->sm_len = spdmem_read(sc, 0);
	s->sm_size = spdmem_read(sc, 1);
	s->sm_type = spdmem_read(sc, 2);

	if (s->sm_type >= SPDMEM_MEMTYPE_FBDIMM) {
		spd_size = 64 << (s->sm_len & SPDMEM_SPDSIZE_MASK);
		switch (s->sm_len & SPDMEM_SPDLEN_MASK) {
		case SPDMEM_SPDLEN_128:
			spd_len = 128;
			break;
		case SPDMEM_SPDLEN_176:
			spd_len = 176;
			break;
		case SPDMEM_SPDLEN_256:
			spd_len = 256;
			break;
		default:
			spd_len = 64;
			break;
		}
	} else {
		spd_size = 1 << s->sm_size;
		spd_len = s->sm_len;
		if (spd_len < 64)
			spd_len = 64;
	}
	if (spd_len > spd_size)
		spd_len = spd_size;
	if (spd_len > sizeof(struct spdmem))
		spd_len = sizeof(struct spdmem);
	for (i = 3; i < spd_len; i++)
		((uint8_t *)s)[i] = spdmem_read(sc, i);

#ifdef DEBUG
	for (i = 0; i < spd_len;  i += 16) {
		int j, k;
		aprint_debug("\n");
		aprint_debug_dev(self, "0x%02x:", i);
		k = (spd_len > i + 16) ? spd_len : i + 16;
		for (j = i; j < k; j++)
			aprint_debug(" %02x", ((uint8_t *)s)[j]);
	}
	aprint_debug("\n");
	aprint_debug_dev(self, "");
#endif

	/*
	 * Setup our sysctl subtree, hw.spdmemN
	 */
	if (hw_node != CTL_EOL)
		sysctl_createv(NULL, 0, NULL, &node,
		    0, CTLTYPE_NODE,
		    device_xname(self), NULL, NULL, 0, NULL, 0,
		    CTL_HW, CTL_CREATE, CTL_EOL);
	if (node != NULL && spd_len != 0)
                sysctl_createv(NULL, 0, NULL, NULL,
                    0,
                    CTLTYPE_STRUCT, "spd_data",
		    SYSCTL_DESCR("raw spd data"), NULL,
                    0, s, spd_len,
                    CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL);

	/*
	 * Decode and print key SPD contents
	 */
	if (IS_RAMBUS_TYPE) {
		if (s->sm_type == SPDMEM_MEMTYPE_RAMBUS)
			type = "Rambus";
		else if (s->sm_type == SPDMEM_MEMTYPE_DIRECTRAMBUS)
			type = "Direct Rambus";
		else
			type = "Rambus (unknown)";

		switch (s->sm_len) {
		case 0:
			rambus_rev = "Invalid";
			break;
		case 1:
			rambus_rev = "0.7";
			break;
		case 2:
			rambus_rev = "1.0";
			break;
		default:
			rambus_rev = "Reserved";
			break;
		}
	} else {
		if (s->sm_type < __arraycount(spdmem_basic_types))
			type = spdmem_basic_types[s->sm_type];
		else
			type = "unknown memory type";

		if (s->sm_type == SPDMEM_MEMTYPE_EDO &&
		    s->sm_fpm.fpm_superset == SPDMEM_SUPERSET_EDO_PEM)
			type = spdmem_superset_types[SPDMEM_SUPERSET_EDO_PEM];
		if (s->sm_type == SPDMEM_MEMTYPE_SDRAM &&
		    s->sm_sdr.sdr_superset == SPDMEM_SUPERSET_SDRAM_PEM)
			type = spdmem_superset_types[SPDMEM_SUPERSET_SDRAM_PEM];
		if (s->sm_type == SPDMEM_MEMTYPE_DDRSDRAM &&
		    s->sm_ddr.ddr_superset == SPDMEM_SUPERSET_DDR_ESDRAM)
			type =
			    spdmem_superset_types[SPDMEM_SUPERSET_DDR_ESDRAM];
		if (s->sm_type == SPDMEM_MEMTYPE_SDRAM &&
		    s->sm_sdr.sdr_superset == SPDMEM_SUPERSET_ESDRAM) {
			type = spdmem_superset_types[SPDMEM_SUPERSET_ESDRAM];
		}
	}

	aprint_normal("\n");
	aprint_normal_dev(self, "%s memory", type);
	strlcpy(sc->sc_type, type, SPDMEM_TYPE_MAXLEN);
	if (node != NULL)
		sysctl_createv(NULL, 0, NULL, NULL,
		    0,
		    CTLTYPE_STRING, "mem_type",
		    SYSCTL_DESCR("memory module type"), NULL,
		    0, sc->sc_type, 0,
		    CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL);

	if (IS_RAMBUS_TYPE)
		aprint_normal(", SPD Revision %s", rambus_rev);
	else if (s->sm_config < __arraycount(spdmem_parity_types) &&
		 (s->sm_type == SPDMEM_MEMTYPE_SDRAM ||
		  s->sm_type == SPDMEM_MEMTYPE_DDRSDRAM ||
		  s->sm_type == SPDMEM_MEMTYPE_DDR2SDRAM))
		aprint_normal(", %s", spdmem_parity_types[s->sm_config]);

	/* Extract module size info */
	dimm_size = 0;
	if (IS_RAMBUS_TYPE) {
		dimm_size = s->sm_rdr.rdr_rows + s->sm_rdr.rdr_cols - 13;
		num_banks = 1;
		per_chip = 1;
	} else if (s->sm_type == SPDMEM_MEMTYPE_SDRAM) {
		dimm_size = s->sm_sdr.sdr_rows + s->sm_sdr.sdr_cols - 17;
		num_banks = s->sm_sdr.sdr_banks;
		per_chip = s->sm_sdr.sdr_banks_per_chip;
	} else if (s->sm_type == SPDMEM_MEMTYPE_DDRSDRAM) {
		dimm_size = s->sm_ddr.ddr_rows + s->sm_ddr.ddr_cols - 17;
		num_banks = s->sm_ddr.ddr_ranks;
		per_chip = s->sm_ddr.ddr_banks_per_chip;
	} else if (s->sm_type == SPDMEM_MEMTYPE_DDR2SDRAM) {
		dimm_size = s->sm_ddr2.ddr2_rows + s->sm_ddr2.ddr2_cols - 17;
		num_banks = s->sm_ddr2.ddr2_ranks + 1;
		per_chip = s->sm_ddr2.ddr2_banks_per_chip;
	} else if (s->sm_type == SPDMEM_MEMTYPE_FBDIMM ||
		   s->sm_type == SPDMEM_MEMTYPE_FBDIMM_PROBE) {
		/*
		 * FB-DIMM is quite different from DDR2
		 */
		dimm_size =
			s->sm_fbd.fbdimm_rows + 12 +
			s->sm_fbd.fbdimm_cols +  9 - 20 - 3;
		num_banks = 1 << (s->sm_fbd.fbdimm_banks + 2);
		per_chip = 1;
	}
	if (IS_RAMBUS_TYPE ||
	    (num_banks <= 8 && per_chip <= 8 && dimm_size > 0 &&
	     dimm_size <= 12)) {
		dimm_size = (1 << dimm_size) * num_banks * per_chip;
		aprint_normal(", %dMB", dimm_size);
		if (node != NULL)
			sysctl_createv(NULL, 0, NULL, NULL,
			    CTLFLAG_IMMEDIATE,
			    CTLTYPE_INT, "size",
			    SYSCTL_DESCR("module size in MB"), NULL,
			    dimm_size, NULL, 0,
			    CTL_HW, node->sysctl_num, CTL_CREATE,
			    CTL_EOL);
	}

	/* Nothing further for RAMBUS memory */
	if (IS_RAMBUS_TYPE) {
		aprint_normal("\n");
		return;
	}

	/* cycle_time is expressed in units of 0.01 ns */
	cycle_time = 0;
	if (s->sm_type == SPDMEM_MEMTYPE_SDRAM)
		cycle_time = s->sm_sdr.sdr_cycle_whole * 100 +
			     s->sm_sdr.sdr_cycle_tenths * 10;
	else if (s->sm_type == SPDMEM_MEMTYPE_DDRSDRAM ||
		 s->sm_type == SPDMEM_MEMTYPE_DDR2SDRAM)
		cycle_time = s->sm_ddr2.ddr2_cycle_whole * 100 +
			     spdmem_cycle_frac[s->sm_ddr2.ddr2_cycle_frac];
	if (cycle_time != 0) {
		/*
		 * cycle time is scaled by a factor of 100 to avoid using
		 * floating point.  Calculate memory speed as the number
		 * of cycles per microsecond.
		 */
		d_clk = 100 * 1000;
		if (s->sm_type == SPDMEM_MEMTYPE_DDR2SDRAM) {
			/* DDR2 uses a dual-pumped clock */
			d_clk *= 2;
			bits = s->sm_ddr2.ddr2_datawidth;
			if ((s->sm_config & 0x03) != 0)
				bits -= 8;
			ddr_type_string = "PC2";
		} else if (s->sm_type == SPDMEM_MEMTYPE_DDRSDRAM) {
			/* DDR uses a dual-pumped clock */
			d_clk *= 2;
			bits = le16toh(s->sm_ddr.ddr_datawidth);
			if (s->sm_config == 1 || s->sm_config == 2)
				bits -= 8;
			ddr_type_string = "PC";
		} else {	/* SPDMEM_MEMTYPE_SDRAM */
			bits = le16toh(s->sm_sdr.sdr_datawidth);
			if (s->sm_config == 1 || s->sm_config == 2)
				bits -= 8;
			ddr_type_string = "PC";
		}
		d_clk /= cycle_time;
		p_clk = d_clk * bits / 8;
		if ((p_clk % 100) >= 50)
			p_clk += 50;
		p_clk -= p_clk % 100;
		aprint_normal(", %dMHz, %s-%d", d_clk, ddr_type_string, p_clk);
		if (node != NULL)
			sysctl_createv(NULL, 0, NULL, NULL,
				       CTLFLAG_IMMEDIATE,
				       CTLTYPE_INT, "speed",
				       SYSCTL_DESCR("memory speed in MHz"),
				       NULL, d_clk, NULL, 0,
				       CTL_HW, node->sysctl_num, CTL_CREATE,
				       CTL_EOL);
	}
	aprint_normal("\n");

	aprint_verbose_dev(self, "");
	switch (s->sm_type) {
	case SPDMEM_MEMTYPE_EDO:
	case SPDMEM_MEMTYPE_FPM:
		aprint_verbose(
		    "%d rows, %d cols, %d banks, %dns tRAC, %dns tCAC\n",
		    s->sm_fpm.fpm_rows, s->sm_fpm.fpm_cols,
		    s->sm_fpm.fpm_banks, s->sm_fpm.fpm_tRAC,
		    s->sm_fpm.fpm_tCAC);
		break;
	case SPDMEM_MEMTYPE_ROM:
		aprint_verbose("%d rows, %d cols, %d banks\n",
			s->sm_rom.rom_rows, s->sm_rom.rom_cols,
			s->sm_rom.rom_banks);
		break;
	case SPDMEM_MEMTYPE_SDRAM:
		aprint_verbose(
		    "%d rows, %d cols, %d banks, %d banks/chip, "
		    "%d.%dns cycle time\n",
		    s->sm_sdr.sdr_rows, s->sm_sdr.sdr_cols, s->sm_sdr.sdr_banks,
		    s->sm_sdr.sdr_banks_per_chip, s->sm_sdr.sdr_cycle_whole,
		    s->sm_sdr.sdr_cycle_tenths);
		break;
	case SPDMEM_MEMTYPE_DDRSDRAM:
		aprint_verbose(
		    "%d rows, %d cols, %d ranks, %d banks/chip, "
		    "%d.%dns cycle time\n",
		    s->sm_ddr.ddr_rows, s->sm_ddr.ddr_cols, s->sm_ddr.ddr_ranks,
		    s->sm_ddr.ddr_banks_per_chip, s->sm_ddr.ddr_cycle_whole,
		    s->sm_ddr.ddr_cycle_tenths);
		break;
	case SPDMEM_MEMTYPE_DDR2SDRAM:
		aprint_verbose(
		    "%d rows, %d cols, %d ranks, %d banks/chip, "
		    "%d.%02dns cycle time\n",
		    s->sm_ddr2.ddr2_rows, s->sm_ddr2.ddr2_cols,
		    s->sm_ddr2.ddr2_ranks + 1, s->sm_ddr2.ddr2_banks_per_chip,
		    s->sm_ddr2.ddr2_cycle_whole,
		    spdmem_cycle_frac[s->sm_ddr2.ddr2_cycle_frac]);
		break;
	default:
		break;
	}

	if (s->sm_voltage < __arraycount(spdmem_voltage_types))
		voltage = spdmem_voltage_types[s->sm_voltage];
	else
		voltage = "unknown";

	if (s->sm_refresh < __arraycount(spdmem_refresh_types))
		refresh = spdmem_refresh_types[s->sm_refresh];
	else
		refresh = "unknown";

	aprint_verbose_dev(self, "voltage %s, refresh time %s",
			voltage, refresh);
	if (s->sm_selfrefresh)
		aprint_verbose(" (self-refreshing)");
	aprint_verbose("\n");
}

static uint8_t
spdmem_read(struct spdmem_softc *sc, uint8_t reg)
{
	uint8_t val;

	iic_acquire_bus(sc->sc_tag,0);
	iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr, &reg, 1,
		 &val, 1, 0);
	iic_release_bus(sc->sc_tag, 0);

	return val;
}

SYSCTL_SETUP(sysctl_spdmem_setup, "sysctl hw.spdmem subtree setup")
{
	const struct sysctlnode *node;

	if (sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "hw", NULL,
	    NULL, 0, NULL, 0,
	    CTL_HW, CTL_EOL) != 0)
		return;

	hw_node = node->sysctl_num;
}
