/*	$NetBSD: bootp.h,v 1.4 1999/07/02 11:31:28 itojun Exp $	*/

/* @(#) Header: bootp.h,v 1.7 95/05/04 17:52:46 mccanne Exp  (LBL) */
/*
 * Bootstrap Protocol (BOOTP).  RFC951 and RFC1048.
 *
 * This file specifies the "implementation-independent" BOOTP protocol
 * information which is common to both client and server.
 *
 * Copyright 1988 by Carnegie Mellon.
 *
 * Permission to use, copy, modify, and distribute this program for any
 * purpose and without fee is hereby granted, provided that this copyright
 * and permission notice appear on all copies and supporting documentation,
 * the name of Carnegie Mellon not be used in advertising or publicity
 * pertaining to distribution of the program without specific prior
 * permission, and notice be given in supporting documentation that copying
 * and distribution is by permission of Carnegie Mellon and Stanford
 * University.  Carnegie Mellon makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */


struct bootp {
	unsigned char	bp_op;		/* packet opcode type */
	unsigned char	bp_htype;	/* hardware addr type */
	unsigned char	bp_hlen;	/* hardware addr length */
	unsigned char	bp_hops;	/* gateway hops */
	u_int32_t	bp_xid;		/* transaction ID */
	unsigned short	bp_secs;	/* seconds since boot began */
	unsigned short	bp_unused;
	struct in_addr	bp_ciaddr;	/* client IP address */
	struct in_addr	bp_yiaddr;	/* 'your' IP address */
	struct in_addr	bp_siaddr;	/* server IP address */
	struct in_addr	bp_giaddr;	/* gateway IP address */
	unsigned char	bp_chaddr[16];	/* client hardware address */
	unsigned char	bp_sname[64];	/* server host name */
	unsigned char	bp_file[128];	/* boot file name */
	unsigned char	bp_vend[64];	/* vendor-specific area */
};

/*
 * UDP port numbers, server and client.
 */
#define	IPPORT_BOOTPS		67
#define	IPPORT_BOOTPC		68

#define BOOTREPLY		2
#define BOOTREQUEST		1


/*
 * Vendor magic cookie (v_magic) for CMU
 */
#define VM_CMU		"CMU"

/*
 * Vendor magic cookie (v_magic) for RFC1048
 */
#define VM_RFC1048	{ 99, 130, 83, 99 }



/*
 * RFC1048 tag values used to specify what information is being supplied in
 * the vendor field of the packet.
 */

#define TAG_PAD			((unsigned char)   0)
#define TAG_SUBNET_MASK		((unsigned char)   1)
#define TAG_TIME_OFFSET		((unsigned char)   2)
#define TAG_GATEWAY		((unsigned char)   3)
#define TAG_TIME_SERVER		((unsigned char)   4)
#define TAG_NAME_SERVER		((unsigned char)   5)
#define TAG_DOMAIN_SERVER	((unsigned char)   6)
#define TAG_LOG_SERVER		((unsigned char)   7)
#define TAG_COOKIE_SERVER	((unsigned char)   8)
#define TAG_LPR_SERVER		((unsigned char)   9)
#define TAG_IMPRESS_SERVER	((unsigned char)  10)
#define TAG_RLP_SERVER		((unsigned char)  11)
#define TAG_HOSTNAME		((unsigned char)  12)
#define TAG_BOOTSIZE		((unsigned char)  13)
#define TAG_END			((unsigned char) 255)
/* RFC1497 tags */
#define	TAG_DUMPPATH		((unsigned char)  14)
#define	TAG_DOMAINNAME		((unsigned char)  15)
#define	TAG_SWAP_SERVER		((unsigned char)  16)
#define	TAG_ROOTPATH		((unsigned char)  17)
#define	TAG_EXTPATH		((unsigned char)  18)
/* RFC1533 tags */
#define TAG_IP_FORWARD		((unsigned char)  19)
#define TAG_IP_SRCRT		((unsigned char)  20)
#define TAG_IP_FILTER		((unsigned char)  21)
#define TAG_IP_REASS		((unsigned char)  22)
#define TAG_IP_TTL		((unsigned char)  23)
#define TAG_IP_PMTUTO		((unsigned char)  24)
#define TAG_IP_PMTUPTAB		((unsigned char)  25)
#define TAG_IPIF_MTU		((unsigned char)  26)
#define TAG_IPIF_LSUBNET	((unsigned char)  27)
#define TAG_IPIF_BADDR		((unsigned char)  28)
#define TAG_IPIF_MDISC		((unsigned char)  29)
#define TAG_IPIF_MSUPP		((unsigned char)  30)
#define TAG_IPIF_RDISC		((unsigned char)  31)
#define TAG_IPIF_RSOLADDR	((unsigned char)  32)
#define TAG_IPIF_SROUTE		((unsigned char)  33)
#define TAG_LINK_TRAILER	((unsigned char)  34)
#define TAG_LINK_ARPTO		((unsigned char)  35)
#define TAG_LINK_ETHER802	((unsigned char)  36)
#define TAG_TCP_DEFTTL		((unsigned char)  37)
#define TAG_TCP_KAINT		((unsigned char)  38)
#define TAG_TCP_KAGARBAGE	((unsigned char)  39)
#define TAG_APP_NISDOM		((unsigned char)  40)
#define TAG_APP_NISOPT		((unsigned char)  41)
#define TAG_APP_NTPSRV		((unsigned char)  42)
#define TAG_VENDOR		((unsigned char)  43)
#define TAG_APP_NB_NS_SERVER	((unsigned char)  44)
#define TAG_APP_NB_DD_SERVER	((unsigned char)  45)
#define TAG_APP_NB_NODETYPE	((unsigned char)  46)
#define TAG_APP_NB_SCOPE	((unsigned char)  47)
#define TAG_APP_X_FS		((unsigned char)  48)
#define TAG_APP_X_DM		((unsigned char)  49)
#define TAG_APP_NISPDOM		((unsigned char)  64)
#define TAG_APP_NISPSRV		((unsigned char)  65)
#define TAG_APP_MIPHA		((unsigned char)  68)
#define TAG_APP_SMTPSRV		((unsigned char)  69)
#define TAG_APP_POP3SRV		((unsigned char)  70)
#define TAG_APP_NNTPSRV		((unsigned char)  71)
#define TAG_APP_HTTPSRV		((unsigned char)  72)
#define TAG_APP_FINGERSRV	((unsigned char)  73)
#define TAG_APP_IRCSRV		((unsigned char)  74)
#define TAG_APP_STREETTALKSRV	((unsigned char)  75)
#define TAG_APP_STREETTALKDA	((unsigned char)  76)
/* (post-)RFC1533 DHCP extensions */
#define TAG_DHCP_REQIPADDR	((unsigned char)  50)
#define TAG_DHCP_LEASETIME	((unsigned char)  51)
#define TAG_DHCP_OVERLOAD	((unsigned char)  52)
#define TAG_DHCP_TFTPSRV	((unsigned char)  66)
#define TAG_DHCP_BOOTFILE	((unsigned char)  67)
#define TAG_DHCP_MSGTYP		((unsigned char)  53)
#define TAG_DHCP_SRVID		((unsigned char)  54)
#define TAG_DHCP_PRMREQ		((unsigned char)  55)
#define TAG_DHCP_MSG		((unsigned char)  56)
#define TAG_DHCP_MAXSIZ		((unsigned char)  57)
#define TAG_DHCP_T1		((unsigned char)  58)
#define TAG_DHCP_T2		((unsigned char)  59)
#define TAG_DHCP_CLASSID	((unsigned char)  60)
#define TAG_DHCP_CLIENTID	((unsigned char)  61)



/*
 * "vendor" data permitted for CMU bootp clients.
 */

struct cmu_vend {
	unsigned char	v_magic[4];	/* magic number */
	u_int32_t	v_flags;	/* flags/opcodes, etc. */
	struct in_addr	v_smask;	/* Subnet mask */
	struct in_addr	v_dgate;	/* Default gateway */
	struct in_addr	v_dns1, v_dns2; /* Domain name servers */
	struct in_addr	v_ins1, v_ins2; /* IEN-116 name servers */
	struct in_addr	v_ts1, v_ts2;	/* Time servers */
	unsigned char	v_unused[24];	/* currently unused */
};


/* v_flags values */
#define VF_SMASK	1	/* Subnet mask field contains valid data */
