#	$NetBSD: Makefile,v 1.6 1997/07/05 05:52:39 darrenr Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c
NOMAN=	yes

.include <bsd.kmod.mk>
