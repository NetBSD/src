#	$NetBSD: Makefile,v 1.7.2.1 1997/09/22 06:33:54 thorpej Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c \
	ip_auth.c ip_log.c
NOMAN=	yes

.include <bsd.kmod.mk>
