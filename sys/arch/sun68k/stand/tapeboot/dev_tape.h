/*	$NetBSD: dev_tape.h,v 1.3 2005/12/11 12:19:29 christos Exp $	*/


int	tape_open(struct open_file *, ...);
int	tape_close(struct open_file *);
int	tape_strategy(void *, int, daddr_t, size_t, void *, size_t *);
int	tape_ioctl(struct open_file *, u_long, void *);

