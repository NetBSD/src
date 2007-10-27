/*	$NetBSD: dev_net.h,v 1.5 2007/10/27 12:21:17 tsutsui Exp $	*/

int	net_open __P((struct open_file *, ...));
int	net_close __P((struct open_file *));
int	net_ioctl __P((struct open_file *, u_long, void *));
int	net_strategy __P((void *, int , daddr_t , size_t, void *, size_t *));

#ifdef SUPPORT_BOOTP
extern int try_bootp;
#endif
