#	$NetBSD: Makefile,v 1.5 1997/05/28 02:10:28 thorpej Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c
NOMAN=	yes

.include <bsd.kmod.mk>
