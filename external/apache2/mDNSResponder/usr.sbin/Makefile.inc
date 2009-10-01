# $NetBSD: Makefile.inc,v 1.2 2009/10/01 16:36:20 tsarna Exp $

.include <bsd.own.mk>

.include "${.PARSEDIR}/../Makefile.inc"

CPPFLAGS+=      -DMDNSD_NOROOT -DMDNSD_USER=\"_mdnsd\"

BINDIR?=	/usr/sbin
