/*	$NetBSD: zs_cons.h,v 1.1 1998/05/05 06:48:52 scottr Exp $	*/

extern int	zsinited;
extern void	*zs_conschan;

void		zs_init __P((void));
int		zs_getc __P((void *arg));
void		zs_putc __P((void *arg, int c));

struct zschan	*zs_get_chan_addr __P((int zsc_unit, int channel));

#ifdef	KGDB
void zs_kgdb_init __P((void));
#endif
