/*	$NetBSD: common.h,v 1.3 1998/10/15 01:00:56 ross Exp $	*/

void init_prom_calls __P((void));
void OSFpal __P((void));
void halt __P((void));
u_int64_t prom_dispatch __P((int, ...));
int cpu_number __P((void));
void switch_palcode __P((void));
