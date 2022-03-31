/*	$NetBSD: opt_rumpkernel.h,v 1.8 2022/03/31 01:36:47 yamaguchi Exp $	*/

#ifndef __NetBSD__
#define __NetBSD__
#endif

#define _KERNEL 1
#define _MODULE 1

#define MODULAR 1
#define MULTIPROCESSOR 1
#define MAXUSERS 32

#define DEBUGPRINT

#define DEFCORENAME "rumpdump"
#define DUMP_ON_PANIC 0

#define INET	1
#define INET6	1
#define GATEWAY	1

#define MPLS	1

#define CAN	1

#define SOSEND_NO_LOAN

#undef PIPE_SOCKETPAIR /* would need uipc_usrreq.c */
#define PIPE_NODIRECT

#define WSEMUL_NO_DUMB
#define WSEMUL_VT100

#define PPPOE_SERVER

#define ALTQ
#define ALTQ_CBQ

#define LACP_NOFDX
