/*	$NetBSD: wdcio.h,v 1.1 1998/11/19 19:44:34 kenh Exp $	*/

#ifndef _SYS_WDCIO_H_
#define _SYS_WDCIO_H_

#include <sys/types.h>
#include <sys/ioctl.h>

typedef struct	wdcreq {
	u_long	flags;		/* info about the request status and type */
	u_char	command;	/* command code */
	u_char	features;	/* feature modifier bits for command */
	u_char	sec_count;	/* sector count */
	u_char	sec_num;	/* sector number */
	u_char	head;		/* head number */
	u_short	cylinder;	/* cylinder/lba address */

	caddr_t	databuf;	/* Pointer to I/O data buffer */
	u_long	datalen;	/* length of data buffer */
	int	timeout;	/* Command timeout */
	u_char	retsts;		/* the return status for the command */
	u_char	error;		/* error bits */
} wdcreq_t;

/* bit defintions for flags */
#define WDCCMD_READ		0x00000001
#define WDCCMD_WRITE		0x00000002

/* definitions for the return status (retsts) */
#define WDCCMD_OK	0x00
#define WDCCMD_TIMEOUT	0x01
#define WDCCMD_ERROR	0x02
#define WDCCMD_DF	0x03

#define WDCIOCCOMMAND	_IOWR('Q', 8, wdcreq_t)

#endif /* _SYS_WDCIO_H_ */
