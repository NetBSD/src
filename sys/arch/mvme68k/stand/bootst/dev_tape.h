/*	$NetBSD: dev_tape.h,v 1.3.40.1 2008/01/21 09:37:46 yamt Exp $	*/


int	tape_open(struct open_file *, ...);
int	tape_close(struct open_file *);
int	tape_strategy(void *, int, daddr_t, size_t, void *, size_t *);
int	tape_ioctl(struct open_file *, u_long, void *);

