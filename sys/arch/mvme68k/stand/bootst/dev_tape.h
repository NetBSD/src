/*	$NetBSD: dev_tape.h,v 1.2.16.1 2000/11/20 20:15:27 bouyer Exp $	*/


int	tape_open __P((struct open_file *, ...));
int	tape_close __P((struct open_file *));
int	tape_strategy __P((void *, int, daddr_t, size_t, void *, size_t *));
int	tape_ioctl __P((struct open_file *, u_long, void *));

