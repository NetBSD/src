#	$NetBSD: Makefile,v 1.9 1998/02/04 07:38:55 mikel Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

CPPFLAGS+= -DIPFILTER_LOG
KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c \
	ip_auth.c ip_log.c
NOMAN=	yes

.include <bsd.kmod.mk>
