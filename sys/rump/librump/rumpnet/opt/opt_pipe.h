/*	$NetBSD: opt_pipe.h,v 1.1.2.2 2008/10/19 22:18:07 haad Exp $	*/

#undef PIPE_SOCKETPAIR /* would need uipc_usrreq.c */
#define PIPE_NODIRECT
