/*	$NetBSD: inetcf.h,v 1.4 2018/01/23 21:06:26 sevan Exp $	*/

 /*
  * @(#) inetcf.h 1.1 94/12/28 17:42:30
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

char *inet_cfg(char *);		/* read inetd.conf file */
void inet_set(char *, int);	/* remember internet service */
int inet_get(char *);		/* look up internet service */

#define WR_UNKNOWN	(-1)		/* service unknown */
#define WR_NOT		1		/* may not be wrapped */
#define WR_MAYBE	2		/* may be wrapped */
#define	WR_YES		3		/* service is wrapped */
