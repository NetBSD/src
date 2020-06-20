/*	$NetBSD: octeon_uboot.h,v 1.3 2020/06/20 02:01:56 simonb Exp $	*/

#ifndef _EVBMIPS_OCTEON_UBOOT_H_
#define	_EVBMIPS_OCTEON_UBOOT_H_

#define	OCTEON_BTDESC_ARGV_MAX		64
#define	OCTEON_BTDESC_SERIAL_LEN	20
#define	OCTEON_BTDESC_DEP2_LEN		6

#define	OCTEON_BTINFO_PADDR_OFFSET	392
#define	OCTEON_BTINFO_SERIAL_LEN	20
#define	OCTEON_BTINFO_MAJOR_VERSION	1
#define	OCTEON_BTINFO_MINOR_VERSION	2

struct octeon_btdesc {
	uint32_t	obt_desc_ver;
	uint32_t	obt_desc_size;
	uint64_t	obt_stack_top;			/* deprecated */
	uint64_t	obt_heap_start;
	uint64_t	obt_heap_end;
	uint64_t	obt_deprecated17;
	uint64_t	obt_deprecated16;
	uint32_t	obt_deprecated18;
	uint32_t	obt_deprecated15;
	uint32_t	obt_deprecated14;
	uint32_t	obt_argc;
	uint32_t	obt_argv[OCTEON_BTDESC_ARGV_MAX];
	uint32_t	obt_flags;			/* deprecated */
	uint32_t	obt_core_mask;			/* deprecated */
	uint32_t	obt_dram_size;			/* deprecated */
	uint32_t	obt_phy_mem_desc_addr;		/* deprecated */
	uint32_t	obt_debugger_flag_addr;
	uint32_t	obt_eclock;
	uint32_t	obt_deprecated10;
	uint32_t	obt_deprecated9;
	uint16_t	obt_deprecated8;
	uint8_t		obt_deprecated7;
	uint8_t		obt_deprecated6;
	uint16_t	obt_deprecated5;
	uint8_t		obt_deprecated4;
	uint8_t		obt_deprecated3;
	uint8_t		obt_deprecated2[OCTEON_BTDESC_SERIAL_LEN];
	uint8_t		obt_deprecated1[OCTEON_BTDESC_DEP2_LEN];
	uint8_t		obt_deprecated0;
	uint64_t	obt_boot_info_addr;
};

struct octeon_btinfo {
	uint32_t	obt_major_version;
	uint32_t	obt_minor_version;

	uint64_t	obt_stack_top;
	uint64_t	obt_heap_base;
	uint64_t	obt_heap_end;
	uint64_t	obt_desc_vaddr;

	uint32_t	obt_ebase_addr;
	uint32_t	obt_stack_size;
	uint32_t	obt_flags;
	uint32_t	obt_core_mask;			/* deprecated in v4 */
	uint32_t	obt_dram_size;			/* in MB */
	uint32_t	obt_phy_mem_desc_addr;
	uint32_t	obt_dbg_flags_base_addr;
	uint32_t	obt_eclock_hz;			/* CPU clock speed */
	uint32_t	obt_dclock_hz;			/* DRAM clock speed */
	uint32_t	obt_reserved0;

	uint16_t	obt_board_type;
	uint8_t		obt_board_rev_major;
	uint8_t		obt_board_rev_minor;
	uint16_t	obt_reserved1;
	uint8_t		obt_reserved2;
	uint8_t		obt_reserved3;
	char		obt_board_serial_number[OCTEON_BTINFO_SERIAL_LEN];

	uint8_t		obt_mac_addr_base[6];
	uint8_t		obt_mac_addr_count;

	/* version minor 1 or newer */
	uint64_t	obt_cf_common_base_addr;	/* paddr */
	uint64_t	obt_cf_attr_base_addr;		/* paddr */
	uint64_t	obt_led_display_base_addr;	/* deprecated */

	/* version minor 2 or newer */
	uint32_t	obt_dfa_ref_clock_hz;		/* DFA ref clock */
	uint32_t	obt_config_flags;

	/* version minor 3 or newer */
	uint64_t	obt_fdt_addr;			/* FDT structure */

	/* version minor 4 or newer */
	uint64_t	obt_ext_core_mask;		/* 64-bit core mask */
};

extern struct octeon_btdesc octeon_btdesc;
extern struct octeon_btinfo octeon_btinfo;

#define	OCTEON_SUPPORTED_DESCRIPTOR_VERSION	7

/* obt_board_type */
#define	BOARD_TYPE_UBIQUITI_E100	20002
#define	BOARD_TYPE_UBIQUITI_E120		20004
#define	BOARD_TYPE_UBIQUITI_E200		20003
#define	BOARD_TYPE_UBIQUITI_E220		20005
#define	BOARD_TYPE_UBIQUITI_E220		20005
#define	BOARD_TYPE_UBIQUITI_E1000		20010
#define	BOARD_TYPE_UBIQUITI_E300		20300

/* obt_config_flags */
#define	CONFIG_FLAGS_PCI_HOST			__BIT(0)
#define	CONFIG_FLAGS_PCI_TARGET			__BIT(1)
#define	CONFIG_FLAGS_DEBUG			__BIT(2)
#define	CONFIG_FLAGS_NO_MAGIC			__BIT(3)
#define	CONFIG_FLAGS_OVERSIZE_TLB_MAPPING	__BIT(4)
#define	CONFIG_FLAGS_BREAK			__BIT(5)


struct octeon_bootmem_desc {
#if BYTE_ORDER == BIG_ENDIAN
	uint32_t	bmd_lock;
	uint32_t	bmd_flags;
	uint64_t	bmd_head_addr;

	uint32_t	bmd_major_version;
	uint32_t	bmd_minor_version;
	uint64_t	bmd_app_data_addr;
	uint64_t	bmd_app_data_size;

	uint32_t	bmd_named_block_num_blocks;
	uint32_t	bmd_named_block_name_len;
	uint64_t	bmd_named_block_array_addr;
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
	uint32_t	bmd_flags;
	uint32_t	bmd_lock;
	uint64_t	bmd_head_addr;

	uint32_t	bmd_minor_version;
	uint32_t	bmd_major_version;
	uint64_t	bmd_app_data_addr;
	uint64_t	bmd_app_data_size;

	uint32_t	bmd_named_block_name_len;
	uint32_t	bmd_named_block_num_blocks;
	uint64_t	bmd_named_block_array_addr;
#endif
};

struct octeon_bootmem_block_header {
	uint64_t	bbh_next_block_addr;
	uint64_t	bbh_size;
};

#endif /* _EVBMIPS_OCTEON_UBOOT_H_ */
