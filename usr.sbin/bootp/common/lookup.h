/*	$NetBSD: lookup.h,v 1.3 2002/07/14 00:26:17 wiz Exp $	*/

/* lookup.h */

#include "bptypes.h"	/* for int32, u_int32 */

extern u_char *lookup_hwa(char *hostname, int htype);
extern int lookup_ipa(char *hostname, u_int32 *addr);
extern int lookup_netmask(u_int32 addr, u_int32 *mask);
