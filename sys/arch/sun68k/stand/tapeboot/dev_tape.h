/*	$NetBSD: dev_tape.h,v 1.1 2001/06/14 12:57:17 fredette Exp $	*/


int	tape_open __P((struct open_file *, ...));
int	tape_close __P((struct open_file *));
int	tape_strategy __P((void *, int, daddr_t, size_t, void *, size_t *));
int	tape_ioctl __P((struct open_file *, u_long, void *));

