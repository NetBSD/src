/*	$NetBSD: dev_tape.h,v 1.3 2000/07/24 18:39:15 jdolecek Exp $	*/


int	tape_open __P((struct open_file *, ...));
int	tape_close __P((struct open_file *));
int	tape_strategy __P((void *, int, daddr_t, size_t, void *, size_t *));
int	tape_ioctl __P((struct open_file *, u_long, void *));

