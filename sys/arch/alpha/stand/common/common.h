/*	$NetBSD: common.h,v 1.7 1999/04/02 03:11:57 cgd Exp $	*/

#define	alpha_pal_imb()	__asm__("imb")

void init_prom_calls __P((void));
void OSFpal __P((void));
void halt __P((void));
u_int64_t prom_dispatch __P((int, ...));
void switch_palcode __P((void));
void close_primary_device __P((int));
