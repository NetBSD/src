#	$NetBSD: Makefile,v 1.1.1.2 1997/05/25 12:07:14 darrenr Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c
NOMAN=	yes

.include <bsd.kmod.mk>
