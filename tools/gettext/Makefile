#	$NetBSD: Makefile,v 1.1 2002/02/26 18:14:40 tv Exp $

.include <bsd.own.mk>

MODULE=		gettext

CONFIGURE_ARGS=	--program-transform-name="s,^,nb,"
MAKE_ARGS=	MAKEINFO=${MAKEINFO:Q}

.include "${.CURDIR}/../Makefile.gnuhost"
