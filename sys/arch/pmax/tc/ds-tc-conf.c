/*	$NetBSD: ds-tc-conf.c,v 1.7 1996/01/29 22:52:39 jonathan Exp $	*/

/*
 * Copyright (c) 1995 Jonathan Stone
 * All rights reserved.
 */

/*
 * 3MIN and 3MAXPLUS turbochannel slots.
 * The kmin (3MIN) and kn03 (3MAXPLUS) have the same number of slots.
 * We can share one configuration-struct table and use two slot-address
 * tables to handle the fact that the turbochannel slot size and base
 * addresses are different on the two machines.
 * (thankfully, the IOCTL ASIC subslots are all the same size on all
 * DECstations with IOASICs.) The devices are listed in the order in which
 *  we should probe and attach them.
 */

#define C(x)	((void *)(u_long)x)

#define TC_SCSI  "PMAZ-AA "
#define TC_ETHER "PMAD-AA "

/*
 * The only builtin Turbonchannel device on the kn03 and kmin
 * is the IOCTL asic, which is mapped into TC slot 3.
 */
const struct tc_builtin tc_kn03_builtins[] = {
	{ "IOCTL   ",	3, 0x0, C(3), /*C(3)*/ }
};

/* 3MAXPLUS TC slot addresses */
static struct tc_slotdesc tc_kn03_slots [4] = {
       	{ KV(KN03_PHYS_TC_0_START), C(0) },  /* slot0 - tc option slot 0 */
	{ KV(KN03_PHYS_TC_1_START), C(1) },  /* slot1 - tc option slot 1 */
	{ KV(KN03_PHYS_TC_2_START), C(2) },  /* slot2 - tc option slot 2 */
	{ KV(KN03_PHYS_TC_3_START), C(3) }   /* slot3 - IO asic on b'board */
};
int tc_kn03_nslots =
    sizeof(tc_kn03_slots) / sizeof(tc_kn03_slots[0]);


/* 3MAXPLUS turbochannel autoconfiguration table */
struct tc_attach_args kn03_tc_desc =
{
	KN03_TC_NSLOTS, tc_kn03_slots,
	1, tc_kn03_builtins,
	tc_ds_ioasic_intr_establish,
	tc_ds_ioasic_intr_disestablish
};

/************************************************************************/

/* 3MIN slot addreseses */
static struct tc_slotdesc tc_kmin_slots [] = {
       	{ KV(KMIN_PHYS_TC_0_START), C(0) },   /* slot0 - tc option slot 0 */
	{ KV(KMIN_PHYS_TC_1_START), C(1) },   /* slot1 - tc option slot 1 */
	{ KV(KMIN_PHYS_TC_2_START), C(2) },   /* slot2 - tc option slot 2 */
	{ KV(KMIN_PHYS_TC_3_START), C(3) }    /* slot3 - IO asic on b'board */
};

int tc_kmin_nslots =
    sizeof(tc_kmin_slots) / sizeof(tc_kmin_slots[0]);

/* 3MIN turbochannel autoconfiguration table */
struct tc_attach_args kmin_tc_desc =
{
	KMIN_TC_NSLOTS, tc_kmin_slots,
	1, tc_kn03_builtins, /*XXX*/
	tc_ds_ioasic_intr_establish,
	tc_ds_ioasic_intr_disestablish
};

/************************************************************************/

/*
 * The builtin Turbonchannel devices on the MAXINE
 * is the IOCTL asic, which is mapped into TC slot 3, and the PMAG-DV
 * xcfb framebuffer, which is built into the baseboard.
 */
const struct tc_builtin tc_xine_builtins[] = {
	{ "IOCTL   ",	3, 0x0, C(3), /*C(3)*/ },
	{ "PMAG-DV ",	4, 0x0, C(4), /*C(4)*/ }
};

/* MAXINE slot addreseses */
static struct tc_slotdesc tc_xine_slots [4] = {
       	{ KV(XINE_PHYS_TC_0_START), C(0) },   /* slot 0 - tc option slot 0 */
	{ KV(XINE_PHYS_TC_1_START), C(1) },   /* slot 1 - tc option slot 1 */
	/*{ KV(-1), C(-1) },*/  /* physical space for ``slot 2'' is reserved */
	{ KV(XINE_PHYS_TC_3_START), C(2) },   /* slot 2 - IO asic on b'board */
	{ KV(XINE_PHYS_CFB_START), C(3) }    /* slot 3 - fb on b'board */
};

int tc_xine_nslots =
    sizeof(tc_xine_slots) / sizeof(tc_xine_slots[0]);

struct tc_attach_args xine_tc_desc =
{
	XINE_TC_NSLOTS, tc_xine_slots,
	2, tc_xine_builtins,
	tc_ds_ioasic_intr_establish,
	tc_ds_ioasic_intr_disestablish
};


/************************************************************************/

/* 3MAX (kn02) turbochannel slots  */
/* slot addreseses */
static struct tc_slotdesc tc_kn02_slots [8] = {
       	{ KV(KN02_PHYS_TC_0_START), C(0)},	/* slot 0 - tc option slot 0 */
	{ KV(KN02_PHYS_TC_1_START), C(1), },	/* slot 1 - tc option slot 1 */
	{ KV(KN02_PHYS_TC_2_START), C(2), },	/* slot 2 - tc option slot 2 */
	{ KV(KN02_PHYS_TC_3_START), C(3), },	/* slot 3 - reserved */
	{ KV(KN02_PHYS_TC_4_START), C(4), },	/* slot 4 - reserved */
	{ KV(KN02_PHYS_TC_5_START), C(5), },	/* slot 5 - SCSI on b`board */
	{ KV(KN02_PHYS_TC_6_START), C(6), },	/* slot 6 - b'board Ether */
	{ KV(KN02_PHYS_TC_7_START), C(7), }	/* slot 7 - system CSR, etc. */
};

int tc_kn02_nslots =
    sizeof(tc_kn02_slots) / sizeof(tc_kn02_slots[0]);

#define KN02_ROM_NAME KN02_ASIC_NAME

#define TC_KN02_DEV_IOASIC     -1
#define TC_KN02_DEV_ETHER	6
#define TC_KN02_DEV_SCSI	5

const struct tc_builtin tc_kn02_builtins[] = {
	{ KN02_ROM_NAME,7, 0x0, C(TC_KN02_DEV_IOASIC) /* C(7)*/ },
	{ TC_ETHER,	6, 0x0, C(TC_KN02_DEV_ETHER)  /* C(6)*/ },
	{ TC_SCSI,	5, 0x0, C(TC_KN02_DEV_SCSI)   /* C(5)*/ }
};


struct tc_attach_args kn02_tc_desc =
{
	8, tc_kn02_slots,
	3, tc_kn02_builtins,	/*XXX*/
	tc_ds_ioasic_intr_establish,
	tc_ds_ioasic_intr_disestablish
};
