/*	$NetBSD: dev_tape.h,v 1.1.24.1 2005/01/24 08:35:03 skrll Exp $	*/


int	tape_open(struct open_file *, ...);
int	tape_close(struct open_file *);
int	tape_strategy(void *, int, daddr_t, size_t, void *, size_t *);
int	tape_ioctl(struct open_file *, u_long, void *);

