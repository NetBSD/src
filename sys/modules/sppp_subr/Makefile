# $NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:09:39 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		sppp_subr
SRCS=		if_spppsubr.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6
#CPPFLAGS+=	-DIPV6CP_MYIFID_DYN	# Will not build if this is enabled!

WARNS=		3

.include <bsd.kmodule.mk>
