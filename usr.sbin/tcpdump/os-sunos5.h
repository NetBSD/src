/*
 * Copyright (c) 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @(#) Header: os-sunos5.h,v 1.5 94/06/12 14:34:23 leres Exp (LBL)
 */

/* Map things in the ether_arp struct */
#define SHA(ap) ((ap)->arp_sha)
#define SPA(ap) ((ap)->arp_spa)
#define THA(ap) ((ap)->arp_tha)
#define TPA(ap) ((ap)->arp_tpa)

#define EDST(ep) ((ep)->ether_dhost.ether_addr_octet)
#define ESRC(ep) ((ep)->ether_shost.ether_addr_octet)

#define	bcopy(s, d, n)	memcpy(d, s, n)
#define major(x)	((int)(((unsigned)(x)>>8)&0377))
#define minor(x)	((int)((x)&0377))
#define	bzero(s, n)	memset(s, 0, n)
#define	bcmp(s1, s2, n)	memcmp(s1, s2, n)
#define setlinebuf(f)	setvbuf(f, NULL, _IOLBF, 0)

/* Prototypes missing in SunOS 5 */
int strcasecmp(const char *, const char *);
