#	$NetBSD: Makefile,v 1.2 2002/12/08 20:20:00 thorpej Exp $

.include <bsd.own.mk>

MODULE=		gettext

CONFIGURE_ARGS=	--program-transform-name="s,^,${_TOOL_PREFIX},"
MAKE_ARGS=	MAKEINFO=${MAKEINFO:Q}

.include "${.CURDIR}/../Makefile.gnuhost"
