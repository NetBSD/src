/*	$NetBSD: so.h,v 1.2 2003/12/06 13:09:01 simonb Exp $	*/

#ifndef _SO_H_INCLUDE
#define	_SO_H_INCLUDE

/* Low level scsi operation codes */
#define	DISK_READ	3
#define	DISK_WRITE	4

struct scsi_args {
	long ptr [8];
};

int	sc_rdwt(int, int, void *, int, int, int);
int	scsialive(int);
int	scsi_tt_read(int, int, void *, u_int, daddr_t, u_int);
void	run_prog(u_long, u_long, u_long, u_long, u_long, u_long);

#endif /* _SO_H_INCLUDE */
