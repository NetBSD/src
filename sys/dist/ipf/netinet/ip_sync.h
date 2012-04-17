/*	$NetBSD: ip_sync.h,v 1.8.12.1 2012/04/17 00:08:14 yamt Exp $	*/

/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * @(#)ip_fil.h	1.35 6/5/96
 * Id: ip_sync.h,v 2.11.2.4 2006/07/14 06:12:20 darrenr Exp
 */

#ifndef __IP_SYNC_H__
#define __IP_SYNC_H__

typedef	struct	synchdr	{
	u_32_t		sm_magic;	/* magic */
	u_char		sm_v;		/* version: 4,6 */
	u_char		sm_p;		/* protocol */
	u_char		sm_cmd;		/* command */
	u_char		sm_table;	/* NAT, STATE, etc */
	u_int		sm_num;		/* table entry number */
	int		sm_rev;		/* forward/reverse */
	int		sm_len;		/* length of the data section */
	struct	synclist	*sm_sl;		/* back pointer to parent */
} synchdr_t;


#define SYNHDRMAGIC 0x0FF51DE5

/*
 * Commands
 * No delete required as expirey will take care of that!
 */
#define	SMC_CREATE	0	/* pass ipstate_t after synchdr_t */
#define	SMC_UPDATE	1
#define	SMC_MAXCMD	1

/*
 * Tables
 */
#define	SMC_NAT		0
#define	SMC_STATE	1
#define	SMC_MAXTBL	1


/*
 * Only TCP requires "more" information than just a reference to the entry
 * for which an update is being made.
 */
typedef	struct	synctcp_update	{
	u_long		stu_age;
	tcpdata_t	stu_data[2];
	int		stu_state[2];
} synctcp_update_t;


typedef	struct	synclist	{
	struct	synclist	*sl_next;
	struct	synclist	**sl_pnext;
	int			sl_idx;		/* update index */
	struct	synchdr		sl_hdr;
	union	{
		struct	ipstate	*slu_ips;
		struct	nat	*slu_ipn;
		void		*slu_ptr;
	} sl_un;
} synclist_t;

#define	sl_ptr	sl_un.slu_ptr
#define	sl_ips	sl_un.slu_ips
#define	sl_ipn	sl_un.slu_ipn
#define	sl_magic sl_hdr.sm_magic
#define	sl_v	sl_hdr.sm_v
#define	sl_p	sl_hdr.sm_p
#define	sl_cmd	sl_hdr.sm_cmd
#define	sl_rev	sl_hdr.sm_rev
#define	sl_table	sl_hdr.sm_table
#define	sl_num	sl_hdr.sm_num
#define	sl_len	sl_hdr.sm_len

/*
 * NOTE: SYNCLOG_SZ is defined *low*.  It should be the next power of two
 * up for whatever number of packets per second you expect to see.  Be
 * warned: this index's a table of large elements (upto 272 bytes in size
 * each), and thus a size of 8192, for example, results in a 2MB table.
 * The lesson here is not to use small machines for running fast firewalls
 * (100BaseT) in sync, where you might have upwards of 10k pps.
 */
#define	SYNCLOG_SZ	256

typedef	struct	synclogent	{
	struct	synchdr	sle_hdr;
	union	{
		struct	ipstate	sleu_ips;
		struct	nat	sleu_ipn;
	} sle_un;
} synclogent_t;

typedef	struct	syncupdent	{		/* 28 or 32 bytes */
	struct	synchdr	sup_hdr;
	struct	synctcp_update	sup_tcp;
} syncupdent_t;

extern	synclogent_t	synclog[SYNCLOG_SZ];


extern	int fr_sync_ioctl(void *, ioctlcmd_t, int, int, void *);
extern	synclist_t *ipfsync_new(int, fr_info_t *, void *);
extern	void ipfsync_del(synclist_t *);
extern	void ipfsync_update(int, fr_info_t *, synclist_t *);
extern	int ipfsync_init(void);
extern	int ipfsync_nat(synchdr_t *sp, void *data);
extern	int ipfsync_state(synchdr_t *sp, void *data);
extern	int ipfsync_read(struct uio *uio);
extern	int ipfsync_write(struct uio *uio);
extern	int ipfsync_canread(void);
extern	int ipfsync_canwrite(void);

#endif /* IP_SYNC */
