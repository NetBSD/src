#	$NetBSD: Makefile,v 1.11 1999/12/12 11:18:46 veego Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

CPPFLAGS+= -DIPFILTER_LOG -I$S/netinet
KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c \
	ip_auth.c ip_log.c
MKMAN=	no

.include <bsd.kmod.mk>
