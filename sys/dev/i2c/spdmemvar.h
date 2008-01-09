/* $NetBSD: spdmemvar.h,v 1.1.4.1 2008/01/09 01:52:42 matt Exp $ */

/*
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

#if BYTE_ORDER == BIG_ENDIAN
#define SPD_BITFIELD(a, b, c, d) d; c; b; a
#else
#define SPD_BITFIELD(a, b, c, d) a; b; c; d
#endif

struct spdmem_fpm {				/* FPM and EDO DIMMS */
	uint8_t fpm_rows;
	uint8_t fpm_cols;
	uint8_t fpm_banks;
	uint16_t fpm_datawidth;			/* endian-sensitive */
	uint8_t fpm_voltage;
	uint8_t	fpm_tRAC;
	uint8_t fpm_tCAC;
	uint8_t fpm_config;
	SPD_BITFIELD(				\
		uint8_t fpm_refresh:7,		\
		uint8_t fpm_selfrefresh:1, ,	\
	);
	uint8_t fpm_dram_dramwidth;
	uint8_t fpm_dram_eccwidth;
	uint8_t	fpm_unused2[17];
	uint8_t	fpm_superset;
	uint8_t fpm_unused3[30];
} __packed;

struct spdmem_sdram {				/* PC66/PC100/PC133 SDRAM */
	SPD_BITFIELD(				\
		uint8_t sdr_rows:4,		\
		uint8_t sdr_rows2:4, ,		\
	);
	SPD_BITFIELD(				\
		uint8_t sdr_cols:4,		\
		uint8_t sdr_cols2:4, ,		\
	);
	uint8_t sdr_banks;
	uint16_t sdr_datawidth;			/* endian-sensitive */
	uint8_t sdr_voltage;
	SPD_BITFIELD(				\
		uint8_t sdr_cycle_tenths:4,	\
		uint8_t sdr_cycle_whole:4, ,	\
	);
	SPD_BITFIELD(
		uint8_t sdr_tAC_tenths:4,	\
		uint8_t	sdr_tAC_whole:4, ,	\
	);
	uint8_t sdr_config;
	SPD_BITFIELD(				\
		uint8_t sdr_refresh:7,		\
		uint8_t sdr_selfrefresh:1, ,	\
	);
	SPD_BITFIELD(				\
		uint8_t sdr_dramwidth:7,	\
		uint8_t sdr_dram_asym_bank2:1, ,\
	);
	SPD_BITFIELD(				\
		uint8_t sdr_eccwidth:7,		\
		uint8_t sdr_ecc_asym_bank2:1, ,	\
	);
	uint8_t sdr_min_clk_delay;
	SPD_BITFIELD(				\
		uint8_t sdr_burstlengths:4,	\
		uint8_t sdr_unused1:4, ,	\
	);
	uint8_t sdr_banks_per_chip;
	uint8_t sdr_tCAS;
	uint8_t sdr_tCS;
	uint8_t sdr_tWE;
	uint8_t sdr_mod_attrs;
	uint8_t sdr_dev_attrs;
	uint8_t sdr_min_cc_1;
	uint8_t sdr_max_tAC_1;
	uint8_t sdr_min_cc_2;
	uint8_t sdr_max_tAC_2;
	uint8_t sdr_tRP;
	uint8_t sdr_tRRD;
	uint8_t sdr_tRCD;
	uint8_t sdr_tRAS;
	uint8_t sdr_module_rank_density;
	uint8_t sdr_tIS;
#define	sdr_superset sdr_tIS
	uint8_t sdr_tIH;
	uint8_t sdr_tDS;
	uint8_t sdr_tDH;
	uint8_t sdr_unused2[5];
	uint8_t sdr_tRC;
	uint8_t	sdr_unused3[20];
	uint8_t	sdr_esdram;
	uint8_t	sdr_super_tech;
	uint8_t	sdr_spdrev;
} __packed;

struct spdmem_rom {
	uint8_t rom_rows;
	uint8_t rom_cols;
	uint8_t rom_banks;
	uint16_t rom_datawidth;			/* endian-sensitive */
	uint8_t rom_voltage;
	uint16_t rom_tAA;			/* endian-sensitive */
	uint8_t rom_config;
	uint8_t	rom_unused1;
	uint8_t	rom_tPA;
	uint8_t rom_tOE;
	uint16_t rom_tCE;			/* endian-sensitive */
	uint8_t	rom_burstlength;
	uint8_t rom_unused2[14];
	uint8_t	rom_superset[31];
} __packed;


struct spdmem_ddr {				/* Dual Data Rate SDRAM */
	SPD_BITFIELD(				\
		uint8_t ddr_rows:4,		\
		uint8_t ddr_rows2:4, ,		\
	);
	SPD_BITFIELD(				\
		uint8_t ddr_cols:4,		\
		uint8_t ddr_cols2:4, ,		\
	);
	uint8_t ddr_ranks;
	uint16_t ddr_datawidth;			/* endian-sensitive */
	uint8_t ddr_voltage;
	SPD_BITFIELD(				\
		uint8_t ddr_cycle_tenths:4,	\
		uint8_t ddr_cycle_whole:4, ,	\
	);
	SPD_BITFIELD(				\
		uint8_t ddr_tAC_hundredths:4,	\
		uint8_t	ddr_tAC_tenths:4, ,	\
	);
	uint8_t ddr_config;
	SPD_BITFIELD(				\
		uint8_t ddr_refresh:7,		\
		uint8_t ddr_selfrefresh:1, ,	\
	);
	SPD_BITFIELD(				\
		uint8_t ddr_dramwidth:7,	\
		uint8_t ddr_dram_asym_bank2:1, ,\
	);
	SPD_BITFIELD(				\
		uint8_t ddr_eccwidth:7,		\
		uint8_t ddr_ecc_asym_bank2:1, ,	\
	);
	uint8_t ddr_min_clk_delay;
	SPD_BITFIELD(				\
		uint8_t ddr_burstlengths:4,	\
		uint8_t ddr_unused1:4, ,	\
	);
	uint8_t ddr_banks_per_chip;
	uint8_t ddr_tCAS;
	uint8_t ddr_tCS;
	uint8_t ddr_tWE;
	uint8_t ddr_mod_attrs;
	uint8_t ddr_dev_attrs;
	uint8_t ddr_min_cc_05;
	uint8_t ddr_max_tAC_05;
	uint8_t ddr_min_cc_1;
	uint8_t ddr_max_tAC_1;
	uint8_t ddr_tRP;
	uint8_t ddr_tRRD;
	uint8_t ddr_tRCD;
	uint8_t ddr_tRAS;
	uint8_t ddr_module_rank_density;
	uint8_t ddr_tIS;
#define	ddr_superset ddr_tIS
	uint8_t ddr_tIH;
	uint8_t ddr_tDS;
	uint8_t ddr_tDH;
	uint8_t	ddr_unused2[5];
	uint8_t ddr_tRC;
	uint8_t ddr_tRFC;
	uint8_t ddr_tCK;
	uint8_t	ddr_tDQSQ;
	uint8_t	ddr_tQHS;
	uint8_t	ddr_unused3;
	uint8_t	ddr_height;
	uint8_t ddr_unused4[15];
} __packed;

struct spdmem_ddr2 {				/* Dual Data Rate 2 SDRAM */
	SPD_BITFIELD(				\
		uint8_t ddr2_rows:5,		\
		uint8_t ddr2_unused1:3,	,	\
	);
	SPD_BITFIELD(				\
		uint8_t ddr2_cols:4,		\
		uint8_t ddr2_unused2:4, ,	\
	);
	SPD_BITFIELD(				\
		uint8_t ddr2_ranks:3,
		uint8_t ddr2_cardoncard:1,	\
		uint8_t ddr2_package:1,		\
		uint8_t ddr2_height:3		\
	);
	uint8_t ddr2_datawidth;
	uint8_t	ddr2_unused3;
	uint8_t ddr2_voltage;
	SPD_BITFIELD(				\
		uint8_t ddr2_cycle_frac:4,	\
		uint8_t ddr2_cycle_whole:4, ,	\
	);
	SPD_BITFIELD(				\
		uint8_t ddr2_tAC_hundredths:4,	\
		uint8_t	ddr2_tAC_tenths:4, ,	\
	);
	uint8_t ddr2_config;
	SPD_BITFIELD(				\
		uint8_t ddr2_refresh:7,		\
		uint8_t ddr2_selfrefresh:1, ,	\
	);
	uint8_t	ddr2_dramwidth;
	uint8_t	ddr2_eccwidth;
	uint8_t	ddr2_unused4;
	SPD_BITFIELD(				\
		uint8_t ddr2_burstlengths:4,	\
		uint8_t ddr2_unused5:4, ,	\
	);
	uint8_t ddr2_banks_per_chip;
	uint8_t ddr2_tCAS;
	uint8_t ddr2_mechanical;
	uint8_t	ddr2_dimm_type;
	uint8_t ddr2_mod_attrs;
	uint8_t ddr2_dev_attrs;
	uint8_t ddr2_min_cc_1;
	uint8_t ddr2_max_tAC_1;
	uint8_t ddr2_min_cc_2;
	uint8_t ddr2_max_tAC_2;
	uint8_t ddr2_tRP;
	uint8_t ddr2_tRRD;
	uint8_t ddr2_tRCD;
	uint8_t ddr2_tRAS;
	uint8_t ddr2_module_rank_density;
	uint8_t ddr2_tIS;
	uint8_t ddr2_tIH;
	uint8_t ddr2_tDS;
	uint8_t ddr2_tDH;
	uint8_t ddr2_tWR;
	uint8_t ddr2_tWTR;
	uint8_t ddr2_tRTP;
	uint8_t ddr2_probe;
	uint8_t	ddr2_extensions;
	uint8_t	ddr2_tRC;
	uint8_t	ddr2_tRFC;
	uint8_t	ddr2_tCK;
	uint8_t	ddr2_tDQSQ;
	uint8_t	ddr2_tQHS;
	uint8_t	ddr2_pll_relock;
	uint8_t	ddr2_Tcasemax;
	uint8_t	ddr2_Psi_TA_DRAM;
	uint8_t	ddr2_dt0;
	uint8_t	ddr2_dt2NQ;
	uint8_t	ddr2_dr2P;
	uint8_t	ddr2_dt3N;
	uint8_t	ddr2_dt3Pfast;
	uint8_t	ddr2_dt3Pslow;
	uint8_t	ddr2_dt4R_4R4W_mode;
	uint8_t	ddr2_dt5B;
	uint8_t	ddr2_dt7;
	uint8_t	ddr2_Psi_TA_PLL;
	uint8_t	ddr2_Psi_TA_Reg;
	uint8_t	ddr2_dt_PLL_Active;
	uint8_t	ddr2_dt_Reg_Active;
	uint8_t ddr2_spdrev;
} __packed;

struct spdmem_fbdimm {				/* Fully-buffered DIMM */
	SPD_BITFIELD(				\
		uint8_t	fbdimm_ps1_voltage:4,	\
		uint8_t	fbdimm_ps2_voltage:4, ,	\
	);
	SPD_BITFIELD(				\
		uint8_t	fbdimm_banks:2,		\
		uint8_t	fbdimm_cols:3,		\
		uint8_t	fbdimm_rows:3,		\
	);
	SPD_BITFIELD(				\
		uint8_t	fbdimm_thick:3,		\
		uint8_t	fbdimm_height:3,	\
		uint8_t	fbdimm_unused1:2,	\
	);
	uint8_t	fbdimm_mod_type;
	SPD_BITFIELD(				\
		uint8_t	fbdimm_dev_width:3,	\
		uint8_t	fbdimm_ranks:3,		\
		uint8_t fbdimm_unused2:2,	\
	);
	SPD_BITFIELD(				\
		uint8_t	fbdimm_ftb_divisor:4,	\
		uint8_t	fbdimm_ftp_dividend:4, ,\
	);
	uint8_t	fbdimm_mtb_dividend;
	uint8_t	fbdimm_mtb_divisor;
	uint8_t	fbdimm_cycle_min;
	uint8_t	fbdimm_cycle_max;
	uint8_t	fbdimm_tCAS;
	uint8_t	fbdimm_tAA_min;
	SPD_BITFIELD(				\
		uint8_t	fbdimm_tWR_min:4,	\
		uint8_t	fbdimm_WR_range:4, ,	\
	);
	uint8_t	fbdimm_tWR;
	SPD_BITFIELD(				\
		uint8_t	fbdimm_tWL_min:4,	\
		uint8_t	fbdimm_tWL_range:4, ,	\
	);
	SPD_BITFIELD(				\
		uint8_t	fbdimm_tAL_min:4,	\
		uint8_t	fbdimm_tAL_range:4, ,	\
	);
	uint8_t	fbdimm_tRCD;
	uint8_t	fbdimm_tRRD;
	uint8_t	fbdimm_tRP;
	SPD_BITFIELD(				\
		uint8_t	fbdimm_tRAS_high:4,	\
		uint8_t	fbdimm_tRC_high:4, ,	\
	);
	uint8_t	fbdimm_tRAS_lo;
	uint8_t	fbdimm_tRC_lo;
	uint16_t fbdimm_tRFC;			/* endian-sensitive */
	uint8_t	fbdimm_tWTR;
	uint8_t	fbdimm_tRTP;
	SPD_BITFIELD(				\
		uint8_t	fbdimm_burst_4:1,	\
		uint8_t	fbdimm_burst_8:1,	\
		uint8_t	fbdimm_unused3:6,	\
	);
	uint8_t	fbdimm_terms;
	uint8_t	fbdimm_drivers;
	uint8_t	fbdimm_tREFI;
	uint8_t	fbdimm_Tcasemax;
	uint8_t	fbdimm_Psi_TA_SDRAM;
	uint8_t	fbdimm_DT0;
	uint8_t	fbdimm_DT2N_DT2Q;
	uint8_t	fbdimm_DT2P;
	uint8_t	fbdimm_DT3N;
	uint8_t	fbdimm_DT4R_DT4R4W;
	uint8_t	fbdimm_DT5B;
	uint8_t	fbdimm_DT7;
	uint8_t	fbdimm_unused4[21];
} __packed;

struct spdmem_rambus {				/* Direct Rambus DRAM */
	SPD_BITFIELD(				\
		uint8_t	rdr_rows:4,		\
		uint8_t	rdr_cols:4, ,		\
	);
} __packed;

struct spdmem {
	uint8_t sm_len;
	uint8_t sm_size;
	uint8_t sm_type;
	union {
		struct spdmem_fbdimm	u1_fbd;
		struct spdmem_fpm	u1_fpm;
		struct spdmem_ddr 	u1_ddr;
		struct spdmem_ddr2	u1_ddr2;
		struct spdmem_sdram	u1_sdr;
		struct spdmem_rambus	u1_rdr;
		struct spdmem_rom	u1_rom;
	} sm_u1;
#define	sm_fbd		sm_u1.u1_fbd
#define	sm_fpm		sm_u1.u1_fpm
#define	sm_ddr		sm_u1.u1_ddr
#define	sm_ddr2		sm_u1.u1_ddr2
#define	sm_rdr		sm_u1.u1_rdr
#define	sm_rom		sm_u1.u1_rom
#define	sm_sdr		sm_u1.u1_sdr
	uint8_t	sm_cksum;
} __packed;

/* some fields are in the same place for all memory types */

#define sm_config	sm_fpm.fpm_config
#define sm_voltage	sm_fpm.fpm_voltage
#define	sm_refresh	sm_fpm.fpm_refresh
#define	sm_selfrefresh	sm_fpm.fpm_selfrefresh

#define SPDMEM_TYPE_MAXLEN 16
struct spdmem_softc {
	struct device sc_dev;
	i2c_tag_t	sc_tag;
	i2c_addr_t	sc_addr;
	struct spdmem	sc_spd_data;
	char		sc_type[SPDMEM_TYPE_MAXLEN];
};
