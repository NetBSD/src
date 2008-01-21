/* $NetBSD: bootrec.h,v 1.1.8.2 2008/01/21 09:39:07 yamt Exp $ */

#include <sys/types.h>

#ifndef __BOOTREC_H__
#define __BOOTREC_H__

#define IPLRECID	0xC9C2D4C1	/* IBMA in EBCIDIC */
#define CONFRECID	0xF8E9DACB	/* no idea */

typedef struct boot_record {
	uint32_t	ipl_record;	/* allways IPLRECID */
	char		pad1[20];
	uint32_t	formatted_cap;	/* sectors in the disk */
	char		floppy_last_head; /* nrof heads -1 */
	char		floppy_last_sec;/* sectors per track starting at 1 */
	char		pad2[6];
	uint32_t	bootcode_len;	/* in sectors, 0 means no boot code */
	uint32_t	bootcode_off;	/* 0 if no bootcode, or byte offset to
					 * first instruction */
	uint32_t	bootpart_start;	/* sec num of boot partition */
	uint32_t	bootprg_start;	/* sec num of boot code, 0 for none */
	uint32_t	bootpart_len;	/* len in sectors of boot part. */
	uint32_t	boot_load_addr;	/* 512 byte boundary load addr */
	char		boot_frag;	/* 0/1 fragmentation allowed */
	char		boot_emul;	/* ROS network emulation flag:
					 * 0x0 => not an emul support image
					 * 0x1 => ROS network emulation code
					 * 0x2 => AIX code supporting ROS emul*/
	char		pad3[2];
	uint16_t	custn_len;	/* sec for customization, normal */
	uint16_t	custs_len;	/* sec for cust. service */
	uint32_t	custn_start;	/* start sec for cust. normal */
	uint32_t	custs_start;	/* start sec for cust. service */
	char		pad4[24];
	uint32_t	servcode_len;	/* bootcode_len for service */
	uint32_t	servcode_off;	/* bootcode_off for service */
	uint32_t	servpart_start;	/* bootpart_start for service */
	uint32_t	servprg_start;	/* bootprg_start for service */
	uint32_t	servpart_len;	/* bootpart_len for service */
	uint32_t	serv_load_addr;	/* boot_load_addr for service */
	char		serv_frag;	/* boot_frag for service */
	char		serv_emul;	/* boot_emul for service */
	char		pad5[2];
	uint32_t	pv_id[4];	/* unique_id for pv_id */
	char		pad6[512 - 128 - 16]; /* 16 for pvid */
} boot_record_t;

typedef struct config_record {
	uint32_t	conf_rec;	/* marks the record as valid */
	int32_t		formatted_cap;	/* sectors in disk */
	uint16_t	pad1;
	char		interleave;
	char		sector_size;	/* bytes per sector * 256 */
	uint16_t	last_cyl;	/* number of cyl-1. total is last_cyl
					 * +2 where the last cyl is the CE */
	char		last_head;	/* nrof heads -1 */
	char		last_sec;	/* nrof sectors -1 */
	char		write_precomp;
	char		device_status;	/* POST crap */
	uint16_t	ce_cyl;		/* diag cylinder */
	uint16_t	eol;		/* defects before disk is done. */
	uint16_t	seek_profile[15]; /* ESDI crap */
	char		mfg_id[3];	/* 0,1 size, 2 == disk maker */
	char		pad2;
	uint32_t	pv_id[4];	/* unique_id for pv_id */
	char		pad3[436];
} config_record_t;
	


#endif /* __BOOTREC_H__ */
