/*	$NetBSD: wired_map.h,v 1.1.18.2 2006/06/21 14:51:13 yamt Exp $	*/

/* currently only framebuffer uses wired map */
#define MIPS3_NWIRED_ENTRY	2
#define MIPS3_WIRED_SIZE	MIPS3_PG_SIZE_MASK_TO_SIZE(MIPS3_PG_SIZE_16M)

#include <mips/wired_map.h>
