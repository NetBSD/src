/*	$NetBSD: inetcf.h,v 1.3 1998/01/09 08:11:48 perry Exp $	*/

 /*
  * @(#) inetcf.h 1.1 94/12/28 17:42:30
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

char *inet_cfg __P((char *));		/* read inetd.conf file */
void inet_set __P((char *, int));	/* remember internet service */
int inet_get __P((char *));		/* look up internet service */

#define WR_UNKNOWN	(-1)		/* service unknown */
#define WR_NOT		1		/* may not be wrapped */
#define WR_MAYBE	2		/* may be wrapped */
#define	WR_YES		3		/* service is wrapped */
