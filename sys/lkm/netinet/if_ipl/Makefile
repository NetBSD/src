#	$NetBSD: Makefile,v 1.10 1999/02/12 05:41:57 cjs Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

CPPFLAGS+= -DIPFILTER_LOG
KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c \
	ip_auth.c ip_log.c
MKMAN=	no

.include <bsd.kmod.mk>
