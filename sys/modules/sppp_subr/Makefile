# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:58 rin Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		sppp_subr
SRCS=		if_spppsubr.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6
#CPPFLAGS+=	-DIPV6CP_MYIFID_DYN	# Will not build if this is enabled!

WARNS=		3

.include <bsd.kmodule.mk>
