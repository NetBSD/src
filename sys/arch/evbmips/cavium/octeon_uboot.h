/*	$NetBSD: octeon_uboot.h,v 1.1.18.2 2017/12/03 11:36:08 jdolecek Exp $	*/

#ifndef _EVBMIPS_OCTEON_UBOOT_H_
#define _EVBMIPS_OCTEON_UBOOT_H_

#define OCTEON_BTINFO_PADDR_OFFSET	392
#define OCTEON_BTINFO_SERIAL_LEN	20
#define OCTEON_BTINFO_MAJOR_VERSION	1
#define OCTEON_BTINFO_MINOR_VERSION	2

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
	uint32_t	obt_core_mask;
	uint32_t	obt_dram_size;			/* in MB */
	uint32_t	obt_phy_mem_desc_addr;
	uint32_t	obt_dbg_flags_base_addr;
	uint32_t	obt_eclock_hz;
	uint32_t	obt_dclock_hz;
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
	uint64_t	obt_led_display_base_addr;

	/* version minor 2 or newer */
	uint32_t	obt_dfa_ref_clock_hz;
	uint32_t	obt_config_flags;

};

extern struct octeon_btinfo octeon_btinfo;

/* obt_board_type */
#define BOARD_TYPE_UBIQUITI_E100	20002

#endif /* _EVBMIPS_OCTEON_UBOOT_H_ */
