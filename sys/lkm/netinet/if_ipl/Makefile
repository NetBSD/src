#	$NetBSD: Makefile,v 1.1.1.7 1997/11/14 08:06:26 mrg Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c \
	ip_auth.c ip_log.c
NOMAN=	yes

.include <bsd.kmod.mk>
