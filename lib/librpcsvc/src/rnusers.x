/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * Find out about remote users
 */

#ifndef RPC_HDR
%#ifndef lint
%/*static char sccsid[] = "from: @(#)rnusers.x 1.2 87/09/20 Copyr 1987 Sun Micro";*/
%/*static char sccsid[] = "from: @(#)rnusers.x	2.1 88/08/01 4.0 RPCSRC";*/
%static char rcsid[] = "$Id: rnusers.x,v 1.2 1993/11/21 18:59:03 brezak Exp $";
%#endif /* not lint */
#endif


#ifdef RPC_HDR
%/*
% * The following structures are used by version 2 of the rusersd protocol.
% * They were not developed with rpcgen, so they do not appear as RPCL.
% */
%
%#define	RUSERSVERS_IDLE 2
%#define	RUSERSVERS 3		/* current version */
%#define	MAXUSERS 100
%
%/*
% * This is the structure used in version 2 of the rusersd RPC service.
% * It corresponds to the utmp structure for BSD sytems.
% */
%struct ru_utmp {
%	char	ut_line[8];		/* tty name */
%	char	ut_name[8];		/* user id */
%	char	ut_host[16];		/* host name, if remote */
%	long	ut_time;		/* time on */
%};
%typedef struct ru_utmp rutmp;
%
%struct utmpidle {
%	struct ru_utmp ui_utmp;
%	unsigned ui_idle;
%};
%
%struct utmpidlearr {
%	struct utmpidle **uia_arr;
%	int uia_cnt;
%};
%typedef struct utmpidlearr utmpidlearr;
%
%#define RUSERSVERS_2 ((u_long)2)
%#ifndef RUSERSPROG
%#define RUSERSPROG ((u_long)100002)
%#endif
%#ifndef RUSERSPROC_NUM
%#define RUSERSPROC_NUM ((u_long)1)
%#endif
%#ifndef RUSERSPROC_NAMES
%#define RUSERSPROC_NAMES ((u_long)2)
%#endif
%#ifndef RUSERSPROC_ALLNAMES
%#define RUSERSPROC_ALLNAMES ((u_long)3)
%#endif
%int xdr_utmpidlearr();
%
#endif
