/*	$NetBSD: common.h,v 1.6 1999/04/01 11:08:39 ross Exp $	*/

void init_prom_calls __P((void));
void OSFpal __P((void));
void halt __P((void));
u_int64_t prom_dispatch __P((int, ...));
int cpu_number __P((void));
void switch_palcode __P((void));
void close_primary_device __P((int));
