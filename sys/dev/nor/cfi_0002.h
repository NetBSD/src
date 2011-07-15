/*	$NetBSD: cfi_0002.h,v 1.1 2011/07/15 19:19:57 cliff Exp $	*/

#ifndef	_DEV_NOR_CFI_0002_H_
#define	_DEV_NOR_CFI_0002_H_

/*
 * CFI Primary Vendor-specific Extended Query structure
 * AMD/Fujitsu Extended Command Set 0002
 */
struct cmdset_0002_query_data {
    uint8_t	pri[3];			/* { 'P', 'R', 'I' } */
    uint8_t	version_maj;		/* major version number (ASCII) */
    uint8_t	version_min;		/* minor version number (ASCII) */
    uint8_t	asupt;			/* Si rev., addr-sensitive unlock */
    uint8_t	erase_susp;		/* erase-suspend */
    uint8_t	sector_prot;		/* sector protect */
    uint8_t	tmp_sector_unprot;	/* temporary sector unprotect */
    uint8_t	sector_prot_scheme;	/* sector protect scheme */
    uint8_t	simul_op;		/* simultaneous operation */
    uint8_t	burst_mode_type;	/* burst mode type */
    uint8_t	page_mode_type;		/* page mode type */
    uint8_t	acc_min;		/* Acc supply min voltage */
    uint8_t	acc_max;		/* Acc supply max voltage */
    uint8_t	wp_prot;		/* WP# protection */
    uint8_t	prog_susp;		/* prpogram suspend */
    uint8_t	unlock_bypass;		/* unlock bypass */
    uint8_t	sss_size;		/* secured silicon sector size (1<<N) */
    uint8_t	soft_feat;		/* software features */
    uint8_t	page_size;		/* page size (1<<N) */
    uint8_t	erase_susp_time_max;	/* erase susp. timeout max, 1<<N usec */
    uint8_t	prog_susp_time_max;	/* prog. susp. timeout max, 1<<N usec */
    uint8_t	embhwrst_time_max;	/* emb hw rst timeout max, 1<<N usec */
    uint8_t	hwrst_time_max;		/* !emb hw rst timeout max, 1<<N usec */
};

/* forward references for prototype(s) */
struct nor_softc;
struct cfi;
struct nor_chip;
struct cfi_chip;

extern void cfi_0002_init(struct nor_softc * const, struct cfi * const,
	struct nor_chip * const);
extern void cfi_0002_print(device_t, struct cfi * const);

#endif	/* _DEV_NOR_CFI_0002_H_ */
