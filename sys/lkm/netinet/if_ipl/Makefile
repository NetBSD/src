#	$NetBSD: Makefile,v 1.2 1997/03/29 01:42:33 thorpej Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c
NOMAN=	yes

.include <bsd.kmod.mk>
