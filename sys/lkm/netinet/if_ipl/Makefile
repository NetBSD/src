#	$NetBSD: Makefile,v 1.7 1997/07/06 22:12:56 thorpej Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c \
	ip_auth.c
NOMAN=	yes

.include <bsd.kmod.mk>
