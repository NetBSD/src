#	$NetBSD: Makefile,v 1.1 1997/01/05 13:26:10 mrg Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c
NOMAN=	yes
CFLAGS+=	-DIPFILTER -DIPFILTER_LKM -DIPFILTER_LOG

CLEANFILES+=	if_ipl

.include <bsd.kmod.mk>
