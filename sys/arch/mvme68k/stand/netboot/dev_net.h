/*	$NetBSD: dev_net.h,v 1.2.32.1 2000/11/20 20:15:30 bouyer Exp $	*/

int	net_open __P((struct open_file *, ...));
int	net_close __P((struct open_file *));
int	net_ioctl __P((struct open_file *, u_long, void *));
int	net_strategy __P((void *, int, daddr_t, size_t, void *, size_t *));
int	net_mountroot __P((struct open_file *, char *));

void	machdep_common_ether __P((u_char *));
