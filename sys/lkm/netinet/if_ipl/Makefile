#	$NetBSD: Makefile,v 1.1.1.3 1997/05/27 22:18:49 thorpej Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c
NOMAN=	yes

.include <bsd.kmod.mk>
