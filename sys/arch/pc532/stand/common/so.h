/*	$NetBSD: so.h,v 1.1 1997/05/17 13:56:11 matthias Exp $	*/

#ifndef _SO_H_INCLUDE
#define _SO_H_INCLUDE

/* Low level scsi operation codes */
#define DISK_READ	3
#define DISK_WRITE	4

struct scsi_args {
	long ptr [8];
};
 
int	sc_rdwt __P((int, int, void *, int, int, int));
int	scsialive __P((int));
int	scsi_tt_read __P((int, int, void *, u_int, daddr_t, u_int));
void	run_prog __P((u_long, u_long, u_long, u_long, u_long, u_long));

#endif /* _SO_H_INCLUDE */
