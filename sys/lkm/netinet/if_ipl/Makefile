#	$NetBSD: Makefile,v 1.3 1997/03/29 03:03:08 thorpej Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c
NOMAN=	yes

CFLAGS+=-DIPFILTER_LKM

.include <bsd.kmod.mk>
