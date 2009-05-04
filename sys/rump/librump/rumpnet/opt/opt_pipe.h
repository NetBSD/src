/*	$NetBSD: opt_pipe.h,v 1.1.16.2 2009/05/04 08:14:31 yamt Exp $	*/

#undef PIPE_SOCKETPAIR /* would need uipc_usrreq.c */
#define PIPE_NODIRECT
