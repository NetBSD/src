#	$NetBSD: Makefile,v 1.8.2.1 1998/07/22 23:06:57 mellon Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

CPPFLAGS+= -DIPFILTER_LOG
KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c \
	ip_auth.c ip_log.c
NOMAN=	yes

.include <bsd.kmod.mk>
