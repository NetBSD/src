/* $NetBSD: iplcb.h,v 1.1.4.2 2008/01/02 21:49:36 bouyer Exp $ */

/* Structure for the IPL Control Block on RS/6000 machines */

#ifndef _IPLCB_H_
#define _IPLCB_H_

#define MAX_BUCS 32

struct ipl_directory {
	char		iplcb_id[8];	/* ascii string id */
	uint32_t	gpr_save_off;	/* offset to gpr_save area */
	uint32_t	cb_bitmap_size;	/* size of bitmap and CB */
	uint32_t	bitmap_off;	/* offset to bitmap */
	uint32_t	bitmap_size;	/* size of bitmap */
	uint32_t	iplinfo_off;
	uint32_t	iplinfo_size;
	uint32_t	iocc_post_off;
	uint32_t	iocc_post_size;
	uint32_t	nio_post_off;
	uint32_t	nio_post_size;
	uint32_t	sjl_post_off;
	uint32_t	sjl_post_size;
	uint32_t	scsi_post_off;
	uint32_t	scsi_post_size;
	uint32_t	eth_post_off;
	uint32_t	eth_post_size;
	uint32_t	tok_post_off;
	uint32_t	tok_post_size;
	uint32_t	ser_post_off;
	uint32_t	ser_post_size;
	uint32_t	par_post_off;
	uint32_t	par_post_size;
	uint32_t	rsc_post_off;
	uint32_t	rsc_post_size;
	uint32_t	lega_post_off;
	uint32_t	lega_post_size;
	uint32_t	kbd_post_off;
	uint32_t	kbd_post_size;
	uint32_t	ram_post_off;
	uint32_t	ram_post_size;
	uint32_t	sga_post_off;
	uint32_t	sga_post_size;
	uint32_t	fm2_post_off;
	uint32_t	fm2_post_size;
	uint32_t	net_boot_result_off;
	uint32_t	net_boot_result_size;
	uint32_t	csc_result_off;
	uint32_t	csc_result_size;
	uint32_t	menu_result_off;
	uint32_t	menu_result_size;
	uint32_t	cons_result_off;
	uint32_t	cons_result_size;
	uint32_t	diag_result_off;
	uint32_t	diag_result_size;
	uint32_t	rom_scan_off;		/* pres of ROMSCAN adaptor */
	uint32_t	rom_scan_size;
	uint32_t	sky_post_off;
	uint32_t	sky_post_size;
	uint32_t	global_off;
	uint32_t	global_size;
	uint32_t	mouse_off;
	uint32_t	mouse_size;
	uint32_t	vrs_off;
	uint32_t	vrs_size;
	uint32_t	taur_post_off;
	uint32_t	taur_post_size;
	uint32_t	ent_post_off;
	uint32_t	ent_post_size;
	uint32_t	vrs40_off;
	uint32_t	vrs40_size;
	uint32_t	gpr_save_area1[64];
	uint32_t	sysinfo_offset;
	uint32_t	sysinfo_size;
	uint32_t	bucinfo_off;
	uint32_t	bucinfo_size;
	uint32_t	procinfo_off;
	uint32_t	procinfo_size;
	uint32_t	fm2_ioinfo_off;
	uint32_t	fm2_ioinfo_size;
	uint32_t	proc_post_off;
	uint32_t	proc_post_size;
	uint32_t	sysvpd_off;
	uint32_t	sysvpd_size;
	uint32_t	memdata_off;
	uint32_t	memdata_size;
	uint32_t	l2data_off;
	uint32_t	l2data_size;
	uint32_t	fddi_post_off;
	uint32_t	fddi_post_size;
	uint32_t	golden_vpd_off;
	uint32_t	golden_vpd_size;
	uint32_t	nvram_cache_off;
	uint32_t	nvram_cache_size;
	uint32_t	user_struct_off;
	uint32_t	user_struct_size;
	uint32_t	residual_off;
	uint32_t	residual_size;
};

struct ipl_cb {
	uint32_t		gpr_save[32];
	struct ipl_directory	dir;
};

struct sys_info {
	int		nrof_procs;
	int		coherency_size;	/* size of coherence block */
	int		resv_size;	/* size of reservation granule */
	u_char		*arb_ctrl_addr;	/* real addr of arbiter control reg */
	u_char		*phys_id_addr;	/* real addr of phys id reg */
	int		nrof_bsrr;	/* nrof 4 byte bos slot reset regs */
	u_char		*bssr_addr;	/* real addr of bssr */
	int		tod_type;	/* time of day type */
	u_char		*todr_addr;	/* real address of todr regs */
	u_char		*rsr_addr;	/* real address of reset status reg */
	u_char		*pksr_addr;	/* RA of power/keylock status reg */
	u_char		*prcr_addr;	/* RA of power/reset control reg */
	u_char		*sssr_addr;	/* RA of system specific regs */
	u_char		*sir_addr;	/* RA of system intr regs */
	u_char		*scr_addr;	/* RA of standard conf reg */
	u_char		*dscr_addr;	/* RA of dev spec. config reg */
	int		nvram_size;	/* in bytes */
	u_char		*nvram_addr;	/* RA of nvram */
	u_char		*vpd_rom_addr;	/* RA of VPD ROM space */
	int		ipl_rom_size;	/* in bytes */
	u_char		*ipl_rom_addr;	/* RA of IPL ROM space */
	u_char		*g_mfrr_addr;	/* RA of global mffr reg if != 0 */
	u_char		*g_tb_addr;	/* RA of global timebase if != 0 */
	int		g_tb_type;	/* global timebase type */
	int		g_tb_mult;	/* global timebase multiplier */
	int		sp_errorlog_off;/* offset from BA of NVRAM to
					 * service processor error log tbl */
	u_char		*pcccr_addr;	/* RA of connectivity config reg */
	u_char		*spocr_addr;	/* RA of software power off ctrl reg */
	u_char		*pfeivr_addr;	/* RA of EPOW ext intr reg */
	/* Available Processor Mask interface */
	int		access_id_waddr;/* type of access to loc_waddr */
	u_char		*loc_waddr;	/* RA of APM space write */
	int		access_id_raddr;/* type of access to loc_raddr */
	u_char		*loc_raddr;	/* RA of APM space read */
	int		architecture;	/* Architecutre of this box:
					 * RS6K = 1 = rs/6000 old mca
					 * RSPC = 2 = PReP */
	int		implementation;
		/* Implementation of this box:
		 * RS6K_UP_MCA = 1 = POWER1, POWER2, RSC, PPC single proc
		 * RS6K_SMP_MCA = 2 = PPC SMP
		 * RSCP_UP_PCI = 3 = PPC/PReP single proc
		 */
	char		pkg_desc[16];	/* NULL term ASCII string: */
		/* "rs6k" POWER1, POWER2, RSC, PPC single proc
		 * "rs6ksmp" PPC SMP
		 * "rspc" PReP
		 */
};

typedef struct buid_data {
	int		buid_value;	/* assigned BUID value.
		* values only have meaning if nrof_buids != 0.
		* assigned in order until nrof_buids is satisfied, unused
		* ones will be -1 
		*/
	u_char		*buid_sptr;	/* pointer to buid's post structure */
} buid_data_t;

struct buc_info {
	uint32_t	nrof_structs;	/* nrof bucs present */
	uint32_t	index;		/* 0 <= index <= num_of_structs - 1 */
	uint32_t	struct_size;	/* in bytes */
	int		bsrr_off;	/* bus slot reset reg offset */
	uint32_t	bsrr_mask;	/* bssr mask */
	int		bscr_val;	/* config register value to enable */
	int		cfg_status;	/* 0 buc not configured
					 * 1 buc configd via config regs
					 * 2 configd via hardware defaults
					 * -1 config failed */
	int		dev_type;	/* 1 buc is executable memory
					 * 2 buc is a processor
					 * 3 buc is an io type */
	int		nrof_buids;	/* nrof buids needed by buc <=4 */
	buid_data_t	buid_data[4];
	int		mem_alloc1;	/* 1st mem alloc required in MB */
	u_char 		*mem_addr1;	/* RA of mem_alloc1 area */
	int		mem_alloc2;	/* 2nd mem alloc required */
	u_char		*mem_addr2;	/* RA of mem_alloc2 area */
	int		vpd_rom_width;	/* width of vpd interface in bytes */
	int		cfg_addr_incr;	/* config addr increment in bytes */
	int		device_id_reg;	/* std. config reg contents */
	uint32_t	aux_info_off;	/* iplcb offset to the device specific
					 * array for this buc. ie, if this is
					 * a proc, offset is the
					 * processor_info struct.
					 */
	uint32_t	feature_rom_code; /* romscan post flag */
	uint32_t	IOCC_flag;	/* 0 not IOCC 1 = IOCC */
	char		location[4];	/* location of the BUC */
};

struct proc_info {
	uint32_t	nrof_structs;
	uint32_t	index;
	uint32_t	struct_size;
	uint32_t	per_buc_info_off; /* iplcb offset to buc_info for me */
	u_char		*proc_int_area;
		/* Base Addr of this proc's intr presentation layer regs
		 * BA+0 (CPPR||XISR without side effects)
		 * BA+4 (CPPR||XISR with side effects)
		 * BA+8 (DSIER)
		 * BA+12 (MFRR)
		 * BA+xx (Additional Optional MFRR's)
		 */
	uint32_t	proc_int_area_size; /* size/4 == nrof intr pres regs */
	int		proc_present;	/* 0 no, -1 dead, 1 running, 2 loop
					 * 3 reset state */
	uint32_t	test_run;	/* which tests were run on proc */
	uint32_t	test_stat;	/* test status */
	int		link;		/* 0 = loop until nonzero !=0 branch to
					 * link addr */
	u_char		*lind_addr;	/* see above */
	union {
		uint32_t 	p_id;	/* unique proc id */
		struct {
			uint16_t	p_nodeid; /* phys NUMA nodeid */
			uint16_t	p_cpuid;  /* phys cpu id */
		} s0;
	} u0;
	int 		architecture;	/* proc arch */
	int		implementation;	/* proc type */
	int		version;	/* proc version */
	int		width;		/* proc data word size */
	int		cache_attrib;	/* bit 0 = cache not, cache is present
					 * bit 1 = separate cache/combined */
	int		coherency_size;	/* size of coherence block */
	int		resv_size;	/* size of reservation granule */
	int		icache_block;	/* L1 icache block size */
	int		dcache_block;	/* L1 dcache block size */
	int		icache_size;	/* L1 icache size */
	int		dcache_size;	/* L1 dcache size */
	int		icache_line;	/* L1 icache line size */
	int		dcache_line;	/* L1 dcache line size */
	int		icache_asc;	/* L1 icache associativity */
	int		dcache_asc;	/* L1 dcache associativity */
	int		L2_cache_size;	/* L2 cache size */
	int		L2_cach_asc;	/* L2 cache associativity */
	int		tlb_attrib;	/* tlb buffer attribute bitfield 
					 * 0  present/not present
					 * 1  separate/combined i/d
					 * 4  proc supports tlbia */
	int		itlb_size;	/* entries */
	int		dtlb_size;
	int		itlb_asc;
	int		dtlb_asc;
	int		slb_attrib;	/* segment lookaside buffer bitfield
					 * 0 slb not/present
					 * 1 separate/combined i/d */
	int		islb_size;	/* entries */
	int		dslb_size;
	int		islb_asc;
	int		dslb_asc;
	int		priv_lck_cnt;	/* spin lock count */
	int		rtc_type;	/* RTC type */
	int		rtcXint;	/* nanosec per timebase tick int mult*/
	int		rtcXfrac;	/* same, but fraction multiplier */
	int		bus_freq;	/* bus clock in Hz */
	int		tb_freq;	/* time base clockfreq */
	char		proc_desc[16];	/* processor name ASCII string */
};

/* One SIMM is a nibble wide and will have the value of:
 * 0x0 == good or 0xf == bad
 */
struct simm_def {
	u_char		simm_7and8;
	u_char		simm_3and4;
	u_char		simm_5and6;
	u_char		simm_1and2;
};

/*
 * The IPL Info structure is mostly good for telling us the cache sizes and
 * model codes.  The whole thing is unreasonably large and verbose.
 */

struct ipl_info {
	/* IPL controller and device interface routine */
	u_char		*iplcnd_ptr;		/* ROM Reserved */
	uint32_t	iplcnd_size;
	/* NVRAM expansion code */
	u_char		*nvram_exp_ptr; 	/* ROM Reserved */
	uint32_t	nvram_exp_size;
	/* IPL ROM stack high addr */
	u_char		*ipl_ros_stack_ptr;	/* ROM Reserved */
	uint32_t	ipl_ros_stack_size;
	/* IPL Record */
	u_char		*ipl_rec_ptr;		/* ROM Reserved */
	uint32_t	ipl_rec_size;
	/* Lowest addr needed by IPL ROM */
	u_char		*ros_workarea_lwm;	/* ROM Reserved */
	/* IPL ROM entry table. t=0, SR15 */
	u_char		*ros_entry_tbl_ptr;	/* ROM Reserved */
	uint32_t	ros_entry_tbl_size;
	/* Memory bit maps nrof bytes per bit.  16K/bit */
	uint32_t	bit_map_bytes_per_bit;	/* ROM Reserved */
	/* Highest addressable real address byte + 1. */
	uint32_t	ram_size;		/* ROM Reserved */
	/*
	 * Model Field:
	 * 0xWWXXYYZZ
	 * WW == 0x00. hardware is SGR ss32 or ss64. (ss is speed in MHz)
	 * 	icache is 8k.
	 *	XX == reserved
	 *	YY == reserved
	 *	ZZ == model code:
	 *		bits 0,1 (low order) == style
	 *			00=Tower 01=Desktop 10=Rack 11=Reserved
	 *		bits 2,3 == relative speed
	 *			00=slow 01=Medium 10=High 11=Fast
	 *		bit 4 == number of combo chips
	 *			0 = 2 chips. 1 = 1 chip.
	 *		bit 5 == Number of DCU's.
	 *			0 = 4 DCU's cache is 64k.
	 *			1 = 2 DCU's cache is 32K.
	 *		bits 6,7 = Reserved
	 * WW != 0x00:
	 * WW == 0x01. Hardware is SGR ss32 or ss64 RS1. (POWER)
	 * WW == 0x02. RSC (RISC Single Chip)
	 * WW == 0x04. POWER2/RS2
	 * WW == 0x08. PowerPC
	 *	XX == Package type
	 *		bits 0,1 (low order) == style
	 *			00=Tower 01=Desktop 10=Rack 11=Entry Server
	 *		bit 2 - AIX Hardware verification test supp (rspc)
	 *		bit 3 - AIX Hardware error log analysis supp (rspc)
	 *		bit 4-7 reserved
	 *	YY == Reserved
	 *	ZZ == Model code. (useless)
	 *	Icache K size is obtained from entry icache.
	 *	Dcache K size is obtained from entry dcache.
	 */
	uint32_t 	model;
	/* Power status and keylock register decode.  IO Addr 0x4000E4.
	 * 32bit reg:
	 * Power Status		bits 0-9
	 * Reserved		bits 10-27
	 * Keylock decode	bits 28-31: (X == don't care)
	 *	28 29 30 31
	 *	 1  1  X  X	Workstation
	 * 	 0  1  X  X	Supermini
	 *	 0  0  X  X	Expansion
	 *	 X  X  1  1	Normal mode
	 *	 X  X  1  0	Service mode
	 *	 X  X  0  1	Secure mode
	 */
	uint32_t	powkey_reg;		/* ROM Reserved */
	/* Set to zero during power on, inc'd with each warm IPL */
	int32_t		soft_reboot_count;
	/* Set and used by IPL controller, all are ROM Reserved */
	int32_t		nvram_section1_valid;	/* 0 if CRC bad */
	int32_t		nvram_exp_valid;	/* 0 if CRC bad */
	u_char		prevboot_dev[36];	/* last normal mode ipl */
	char		reserved[28];
	/* Pointer to the IPLCB in memory */
	u_char		*iplcb_ptr;		/* ROM Reserved */
	/* Pointer to compressed BIOS code */
	u_char		*bios_ptr;		/* ROM Reserved */
	uint32_t	bios_size;
	uint32_t	cre[16];	/* Storage Configuration Registers. */
	uint32_t	bscr[16];	/* Bit steering registers */
	/* Unimplemented and ROM Reserved */
	struct {
		uint32_t	synd;
		uint32_t	addr;
		uint32_t	status;
	} single_bit_error[16];
	uint32_t	reserved_array[5*16];
	/* Memory extent test indicators */
	u_char		extent_test_ind[16];	/* 0 = untested, 1 = tested */
	/* Memory bit steering register settig conflict indicator */
	u_char		bit_steer_conflict[16];	/* 1 = conflict */
	/* Set by IPL controller, ROM Reserved */
	uint32_t	ipl_ledval;	/* IPL LED value */
	uint32_t	ipl_device;	/* ??? */
	char		unused[18];

	/* Copied from IPL Rom VPD area */
	char		vpd_planar_partno[8];
	char		vpd_planar_ecno[8];
	char		vpd_proc_serialno[8];
	char		vpd_ipl_ros_partno[8];
	char		vpd_ipl_ros_version[14];
	char		ipl_ros_copyright[49];
	char		ipl_ros_timestamp[10];

	/* Copied from NVRAM */
	union {
		uint32_t	chip_signature;
		struct chip_sig {
			u_char	cop_bus_addr;
			u_char	obsolete_u_num;
			u_char	dd_num;
			u_char	partno;
		} chip_sig;
	} floating_point, fixed_point, instruction_cache_unit,
	  storage_control_unit, combo_1, combo_2, data_control_unit_0,
	  data_control_unit_1, data_control_unit_2, data_control_unit_3;

	/* Memory SIMM error information.
	 * 8 cards (A-H) 8 simms per card (1-8).
	 * Two cache line sizes.  if 128 use memory_card_1n data,
	 * otherwise use memory_card_9n or memory_card_Tn.
	 */
	union {
		char		memcd_errinfo[32];
		uint32_t	slots_of_simm[8];
		struct {	/* cache line size 128 */
			struct simm_def	memory_card_1H;
			struct simm_def	memory_card_1F;
			struct simm_def	memory_card_1G;
			struct simm_def	memory_card_1E;
			struct simm_def	memory_card_1D;
			struct simm_def	memory_card_1B;
			struct simm_def	memory_card_1C;
			struct simm_def	memory_card_1A;
		} simm_stuff_1;
		struct {	/* cache line size 64 */
			struct simm_def memory_card_9H;
			struct simm_def memory_card_9D;
			struct simm_def memory_card_9F;
			struct simm_def memory_card_9B;
			struct simm_def memory_card_9G;
			struct simm_def memory_card_9C;
			struct simm_def memory_card_9E;
			struct simm_def memory_card_9A;
		} simm_stuff_9;
		struct {	/* cache line size 64 */
			struct simm_def memory_card_TB;
			struct simm_def memory_card_TC;
		} simm_stuff_T;
	} simm_info;

	/* MESR error info at IPL ROM memory config time.
	 * Two cache line sizes, similar to above.
	 *   find one of the following values in the word variable:
	 *	0x00000000 no MESR error occurred when configuring this extent
	 *	0xe0400000 timeout. no memcard is in the slot.
	 *	otherwise, an error occurred, so the card exists but will not
	 *	be used.
	 */
	union {
		char	extent_errinfo[64];
		struct {  /* cacheline 128 */
			uint32_t	ext_0_slot_HandD;
			uint32_t	ext_1_slot_HandD;
			uint32_t	ext_2_slot_HandD;
			uint32_t	ext_3_slot_HandD;
			uint32_t	ext_4_slot_FandB;
			uint32_t	ext_5_slot_FandB;
			uint32_t	ext_6_slot_FandB;
			uint32_t	ext_7_slot_FandB;
			uint32_t	ext_8_slot_GandC;
			uint32_t	ext_9_slot_GandC;
			uint32_t	ext_10_slot_GandC;
			uint32_t	ext_11_slot_GandC;
			uint32_t	ext_12_slot_EandA;
			uint32_t	ext_13_slot_EandA;
			uint32_t	ext_14_slot_EandA;
			uint32_t	ext_15_slot_EandA;
		} MESR_err_1;
		struct { /* cacheline of 64 */
			uint32_t	ext_0_slot_H;
			uint32_t	ext_1_slot_H;
			uint32_t	ext_2_slot_D;
			uint32_t	ext_3_slot_D;
			uint32_t	ext_4_slot_F;
			uint32_t	ext_5_slot_F;
			uint32_t	ext_6_slot_B;
			uint32_t	ext_7_slot_B;
			uint32_t	ext_8_slot_G;
			uint32_t	ext_9_slot_G;
			uint32_t	ext_10_slot_C;
			uint32_t	ext_11_slot_C;
			uint32_t	ext_12_slot_E;
			uint32_t	ext_13_slot_E;
			uint32_t	ext_14_slot_A;
			uint32_t	ext_15_slot_A;
		} MESR_err_9;
	} config_err_info;
	char		unused_errinfo[64];

	/*
	 * Memory card VPD data area
	 * cacheline sizes like above.
	 *	0xffffffff = card is present, no VPD
	 *	0x22222222 = card present with errors
	 *	0x11111111 = no card
	 *	otherwise it's good.
	 */
	union {
		char	memcd_vpd[128];
		struct {  /* cacheline 128 */
			char	ext_0_HandD[20];
			char	ext_4_FandB[20];
			char	ext_8_GandC[20];
			char	ext_12_EandA[20];
		} memory_vpd_1;
		struct { /* cacheline 64 */
			char	ext_0_slot_H[10];
			char	dmy_0[2];
			char	ext_2_slot_D[10];
			char	dmy_2[2];
			char	ext_4_slot_F[10];
			char	dmy_4[2];
			char	ext_6_slot_B[10];
			char	dmy_6[2];
			char	ext_8_slot_G[10];
			char	dmy_8[2];
			char	ext_10_slot_C[10];
			char	dmy_10[2];
			char	ext_12_slot_E[10];
			char	dmy_12[2];
			char	ext_14_slot_A[10];
			char	dmy_14[2];
		} memory_vpd_9;
	} memcd_vpd;

	int32_t		cache_line_size;
	/* Component reset register test results for BUID 20.
	 * Anything other than 0x00AA55FF is horked.
	 */
	int32_t		CRR_results;
	/* IO planar level register
	 * 0x1YYXXXXX = family 2
	 * 0x8YYXXXXX = table/desktop    YY is engineering level, X reserved
	 * -1 == not present
	 * Values in MSB has following meaning:
	 *	0x80 = table/desktop
	 *	0x40 = reserved
	 *	0x20 = reserved
	 *	0x10 = rack planar
	 *	0x08 = standard IO
	 *	0x04 = power connector not connected
	 *	0x02, 0x01 reserved
	 */
	int32_t		io_planar_level_reg; /* BUID 20 */
	int32_t		io_planar_level_reg_21;
	int32_t		io_planar_level_reg_22;
	int32_t		io_planar_level_reg_23;

	/* Component regsiter test results for the other BUID's */
	int32_t		CRR_results_21;
	int32_t		CRR_results_22;
	int32_t		CRR_results_23;

	/* CRR results for BUID 20 */
	int32_t		CRR_results_20_0;	/* should contain 0x00000000 */
	int32_t		CRR_results_20_a;	/* should contain 0xaaaaaaaa */
	int32_t		CRR_results_20_5;	/* should contain 0x55555555 */
	int32_t		CRR_results_20_f;	/* should contain 0xffffffff */
	int32_t		CRR_results_21_0;	/* should contain 0x00000000 */
	int32_t		CRR_results_21_a;	/* should contain 0xaaaaaaaa */
	int32_t		CRR_results_21_5;	/* should contain 0x55555555 */
	int32_t		CRR_results_21_f;	/* should contain 0xffffffff */
	int32_t		CRR_results_22_0;	/* should contain 0x00000000 */
	int32_t		CRR_results_22_a;	/* should contain 0xaaaaaaaa */
	int32_t		CRR_results_22_5;	/* should contain 0x55555555 */
	int32_t		CRR_results_22_f;	/* should contain 0xffffffff */
	int32_t		CRR_results_23_0;	/* should contain 0x00000000 */
	int32_t		CRR_results_23_a;	/* should contain 0xaaaaaaaa */
	int32_t		CRR_results_23_5;	/* should contain 0x55555555 */
	int32_t		CRR_results_23_f;	/* should contain 0xffffffff */

	/* IO interrupt test results for BUID 21 */
	int32_t		io_intr_results_21;
	/* pointer to IPL rom code in mem */
	u_char		*rom_ram_addr;		/* ROM Reserved */
	uint32_t	rom_ram_size;
	/* Storage control config register, ROM Reserved */
	uint32_t	sccr_toggle_one_meg;
	/* read from OCS NVRAM area */
	uint32_t	aix_model_code; /* 4 bytes from 0xA0003d0 */
	/* The following entries are read from the OCS NVRAM area
	 * ---------: dcache size = 0x0040     icache size = 0x0008
	 * ---------: dcache size = 0x0020     icache size = 0x0008
	 * ---------: dcache size = 0x0008     icache size = 0x0008
	 * ---------: dcache size = 0x0040     icache size = 0x0020
	 * ---------: dcache size = 0x0020     icache size = 0x0008
	 */
	int32_t		dcache_size; /* 4 bytes from NVRAM 0xA0003d4 */
	int32_t		icache_size; /* 4 bytes from NVRAM address 0xA0003d8 */
	char		vpd_model_id[8];

	/* saves the ptr to lowest addr needed by IPL rom */
	u_char		*low_boundary_save;	/* ROM Reserved */
	/* Pointer to romscan entry point and data area. ROM Reserved */
	u_char		*romscan_code_ptr; /* runtime entry point of rom scan */
	u_char		*rom_boot_data_area; /* runtime user ram ( >= 4K)*/
};




#endif /* _IPLCB_H_ */
