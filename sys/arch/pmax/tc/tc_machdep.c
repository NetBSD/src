/* $NetBSD: tc_machdep.c,v 1.1.2.1 1998/10/15 02:49:01 nisimura Exp $ */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: tc_machdep.c,v 1.1.2.1 1998/10/15 02:49:01 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <dev/tc/tcvar.h>
#include <pmax/tc/ioasicreg.h>

void ioasic_intr_establish __P((struct device *, void *,
	    int, int (*)(void *), void *));
void kmin_intr_establish __P((struct device *, void *,
	    int, int (*)(void *), void *));
void kn02_intr_establish __P((struct device *, void *,
	    int, int (*)(void *), void *));
void ioasic_intr_disestablish __P((struct device *, void *));
void kn02_intr_disestablish __P((struct device *, void *));

/* XXX XXX XXX */
#include <pmax/pmax/maxine.h>
#define XINE_PHYS_TC_0	XINE_PHYS_TC_0_START
#define XINE_PHYS_TC_1	XINE_PHYS_TC_1_START
#define XINE_PHYS_TC_3	XINE_PHYS_TC_3_START
#define XINE_PHYS_XCFB	XINE_PHYS_CFB_START

#include <pmax/pmax/kn03.h>
#define KN03_PHYS_TC_0	KN03_PHYS_TC_0_START
#define KN03_PHYS_TC_1	KN03_PHYS_TC_1_START
#define KN03_PHYS_TC_2	KN03_PHYS_TC_2_START
#define KN03_PHYS_TC_3	KN03_PHYS_TC_3_START

#include <pmax/pmax/kmin.h>
#define	KMIN_PHYS_TC_0	KMIN_PHYS_TC_0_START
#define	KMIN_PHYS_TC_1	KMIN_PHYS_TC_1_START
#define	KMIN_PHYS_TC_2	KMIN_PHYS_TC_2_START
#define	KMIN_PHYS_TC_3	KMIN_PHYS_TC_3_START

#include <pmax/pmax/kn02.h>
#define KN02_PHYS_TC_0	KN02_PHYS_TC_0_START
#define KN02_PHYS_TC_1	KN02_PHYS_TC_1_START
#define KN02_PHYS_TC_2	KN02_PHYS_TC_2_START
#define KN02_PHYS_TC_3	KN02_PHYS_TC_3_START
#define KN02_PHYS_TC_4	KN02_PHYS_TC_4_START
#define KN02_PHYS_TC_5	KN02_PHYS_TC_5_START
#define KN02_PHYS_TC_6	KN02_PHYS_TC_6_START
#define KN02_PHYS_TC_7	KN02_PHYS_TC_7_START
/* XXX XXX XXX */

#define	KV(x)	MIPS_PHYS_TO_KSEG1(x)
#define	C(x)	(void *)(x)

#include "opt_dec_3maxplus.h"
#include "opt_dec_maxine.h"
#include "opt_dec_3min.h"
#include "opt_dec_3max.h"

#if (DEC_MAXINE + DEC_3MAXPLUS + DEC_3MIN) > 0
struct tc_builtin tc_ioasic_builtins[] = {
	{ "IOCTL   ",	3, 0x0, C(SYS_DEV_BOGUS), },
	{ "PMAG-DV ",	2, 0x0, C(SYS_DEV_BOGUS), },
};
#endif

#ifdef DEC_MAXINE
struct tc_slotdesc tc_maxine_slots[] = {
	{ KV(XINE_PHYS_TC_0), C(SYS_DEV_OPT0), },	/* 0 - opt slot 0 */
	{ KV(XINE_PHYS_TC_1), C(SYS_DEV_OPT1), },	/* 1 - opt slot 1 */
	{ KV(XINE_PHYS_XCFB), C(SYS_DEV_BOGUS), },	/* 2 - for xcfb */
	{ KV(XINE_PHYS_TC_3), C(SYS_DEV_BOGUS), },	/* 3 - IOASIC */
};

struct tcbus_attach_args xine_tc_desc = {
	"tc", 0,
	TC_SPEED_12_5_MHZ,
	4, tc_maxine_slots,
	2, tc_ioasic_builtins,
	ioasic_intr_establish, ioasic_intr_disestablish
};
#endif

#ifdef DEC_3MAXPLUS
struct tc_slotdesc tc_kn03_slots[] = {
	{ KV(KN03_PHYS_TC_0), C(SYS_DEV_OPT0), },	/* 0 - opt slot 0 */
	{ KV(KN03_PHYS_TC_1), C(SYS_DEV_OPT1), },	/* 1 - opt slot 1 */
	{ KV(KN03_PHYS_TC_2), C(SYS_DEV_OPT2), },	/* 2 - opt slot 2 */
	{ KV(KN03_PHYS_TC_3), C(SYS_DEV_BOGUS), },	/* 3 - IOASIC */
};

struct tcbus_attach_args kn03_tc_desc = {
	"tc", 0,
	TC_SPEED_25_MHZ,
	4, tc_kn03_slots,
	1, tc_ioasic_builtins,
	ioasic_intr_establish, ioasic_intr_disestablish
};
#endif

#ifdef DEC_3MIN
struct tc_slotdesc tc_kmin_slots[] = {
	{ KV(KMIN_PHYS_TC_0), C(SYS_DEV_OPT0), },	/* 0 - opt slot 0 */
	{ KV(KMIN_PHYS_TC_1), C(SYS_DEV_OPT1), },	/* 1 - opt slot 1 */
	{ KV(KMIN_PHYS_TC_2), C(SYS_DEV_OPT2), },	/* 2 - opt slot 2 */
	{ KV(KMIN_PHYS_TC_3), C(SYS_DEV_BOGUS), },	/* 3 - IOASIC */
};

struct tcbus_attach_args kmin_tc_desc = {
	"tc", 0,
	TC_SPEED_12_5_MHZ,
	4, tc_kmin_slots,
	1, tc_ioasic_builtins,
	kmin_intr_establish, ioasic_intr_disestablish
};
#endif

#ifdef DEC_3MAX
struct tc_slotdesc tc_kn02_slots[] = {
       	{ KV(KN02_PHYS_TC_0), C(SYS_DEV_OPT0), },	/* 0 - opt slot 0 */
       	{ KV(KN02_PHYS_TC_1), C(SYS_DEV_OPT1), },	/* 1 - opt slot 1 */
       	{ KV(KN02_PHYS_TC_2), C(SYS_DEV_OPT2), },	/* 2 - opt slot 2 */
	{ KV(KN02_PHYS_TC_3), C(SYS_DEV_BOGUS),	},	/* 3 - reserved */
	{ KV(KN02_PHYS_TC_4), C(SYS_DEV_BOGUS),	},	/* 4 - reserved */
	{ KV(KN02_PHYS_TC_5), C(SYS_DEV_SCSI),	},	/* 5 - b`board SCSI */
	{ KV(KN02_PHYS_TC_6), C(SYS_DEV_LANCE), },	/* 6 - b`board LANCE */
	{ KV(KN02_PHYS_TC_7), C(SYS_DEV_BOGUS), },	/* 7 - system CSRs */
};
struct tc_builtin tc_kn02_builtins[] = {
	{ "KN02SYS ",	7, 0x0, C(SYS_DEV_BOGUS), },
	{ "PMAD-AA ",	6, 0x0, C(SYS_DEV_LANCE), },
	{ "PMAZ-AA ",	5, 0x0, C(SYS_DEV_SCSI), },
};

struct tcbus_attach_args kn02_tc_desc = {
	"tc", 0,
	TC_SPEED_25_MHZ,
	3, tc_kn02_slots,
	3, tc_kn02_builtins,
	kn02_intr_establish, kn02_intr_disestablish
};
#endif
