/*	$NetBSD: bootinfo.h,v 1.1.2.2 2002/02/28 04:10:57 nathanw Exp $	*/

#ifndef _MVMEPPC_BOOTINFO
#define _MVMEPPC_BOOTINFO

#define	BOOTLINE_LEN	32
#define	CONSOLEDEV_LEN	16

struct mvmeppc_bootinfo {
	u_int32_t	bi_boothowto;
	u_int32_t	bi_bootaddr;
	u_int16_t	bi_bootclun;
	u_int16_t	bi_bootdlun;
	char		bi_bootline[BOOTLINE_LEN];
	char		bi_consoledev[CONSOLEDEV_LEN];
	u_int32_t	bi_consoleaddr;
	u_int32_t	bi_consolechan;
	u_int32_t	bi_consolespeed;
	u_int32_t	bi_consolecflag;
	u_int16_t	bi_modelnumber;
	u_int32_t	bi_memsize;
	u_int32_t	bi_mpuspeed;
	u_int32_t	bi_busspeed;
	u_int32_t	bi_clocktps;
};

#ifdef _KERNEL
extern struct mvmeppc_bootinfo bootinfo;
#endif

#endif /* _MVMEPPC_BOOTINFO */
