/*	$NetBSD: ipv6cp.h,v 1.2 1999/08/25 02:07:43 christos Exp $	*/

/*
 * ipv6cp.h - IP Control Protocol definitions.
 *
 * Derived from :
 *
 *
 * ipcp.h - IP Control Protocol definitions.
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Id: ipv6cp.h,v 1.1 1999/08/13 01:58:43 paulus Exp 
 *
 *
 * Original version by Inria (www.inria.fr)
 * Modified to match RFC2472 by Tommi Komulainen <Tommi.Komulainen@iki.fi>
 */

/*
 * Options.
 */
#define CI_IFACEID	1	/* Interface Identifier */
#define CI_COMPRESSTYPE	2	/* Compression Type     */

/* No compression types yet defined.
 *#define IPV6CP_COMP	0x004f
 */
typedef struct ipv6cp_options {
    int neg_ifaceid;		/* Negotiate interface identifier? */
    int req_ifaceid;		/* Ask peer to send interface identifier? */
    int accept_local;		/* accept peer's value for iface id? */
    int opt_local;		/* ourtoken set by option */
    int opt_remote;		/* histoken set by option */
    int use_ip;			/* use IP as interface identifier */
    int neg_vj;			/* Van Jacobson Compression? */
    u_short vj_protocol;	/* protocol value to use in VJ option */
    eui64_t ourid, hisid;	/* Interface identifiers */
} ipv6cp_options;

extern fsm ipv6cp_fsm[];
extern ipv6cp_options ipv6cp_wantoptions[];
extern ipv6cp_options ipv6cp_gotoptions[];
extern ipv6cp_options ipv6cp_allowoptions[];
extern ipv6cp_options ipv6cp_hisoptions[];

extern struct protent ipv6cp_protent;
