/*	$NetBSD: netbios.h,v 1.1.1.1 1997/10/03 17:25:12 christos Exp $	*/

/*
 * NETBIOS protocol formats
 *
 * @(#) Header: netbios.h,v 1.1 94/06/09 11:47:15 mccanne Exp 
 */

struct p8022Hdr {
    u_char	dsap;
    u_char	ssap;
    u_char	flags;
};

#define	p8022Size	3		/* min 802.2 header size */

#define UI		0x03		/* 802.2 flags */

