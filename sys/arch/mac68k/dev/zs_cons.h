/*	$NetBSD: zs_cons.h,v 1.3.2.2 2005/01/16 00:33:00 chs Exp $	*/

extern int	zsinited;
extern void	*zs_conschan;

void		zs_init(void);
int		zs_getc(void *);
void		zs_putc(void *, int);

struct zschan	*zs_get_chan_addr(int);

#ifdef	KGDB
void zs_kgdb_init(void);
#endif
