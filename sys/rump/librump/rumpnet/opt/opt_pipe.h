/*	$NetBSD: opt_pipe.h,v 1.1.10.2 2009/01/17 13:29:37 mjf Exp $	*/

#undef PIPE_SOCKETPAIR /* would need uipc_usrreq.c */
#define PIPE_NODIRECT
