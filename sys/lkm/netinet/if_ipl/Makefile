#	$NetBSD: Makefile,v 1.1.1.1 1997/03/27 15:14:21 darrenr Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c
NOMAN=	yes

.include <bsd.kmod.mk>
