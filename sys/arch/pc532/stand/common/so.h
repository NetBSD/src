/*	$NetBSD: so.h,v 1.1.54.2 2004/09/18 14:38:35 skrll Exp $	*/

#ifndef _SO_H_INCLUDE
#define	_SO_H_INCLUDE

/* Low level scsi operation codes */
#define	DISK_READ	3
#define	DISK_WRITE	4

struct scsi_args {
	long ptr[8];
};

int	sc_rdwt(int, int, void *, int, int, int);
int	scsialive(int);
int	scsi_tt_read(int, int, void *, u_int, daddr_t, u_int);
int	scsi_tt_write(int, int, void *, u_int, daddr_t, u_int);
void	run_prog(u_long, u_long, u_long, u_long, u_long, u_long);
int	exec_scsi_low(struct scsi_args *, long);

#endif /* _SO_H_INCLUDE */
