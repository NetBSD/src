#	$NetBSD: Makefile,v 1.8 1997/09/21 18:03:55 veego Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c \
	ip_auth.c ip_log.c
NOMAN=	yes

.include <bsd.kmod.mk>
