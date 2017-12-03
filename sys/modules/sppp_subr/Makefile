# $NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:59 jdolecek Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		sppp_subr
SRCS=		if_spppsubr.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6
#CPPFLAGS+=	-DIPV6CP_MYIFID_DYN	# Will not build if this is enabled!

.include <bsd.kmodule.mk>
