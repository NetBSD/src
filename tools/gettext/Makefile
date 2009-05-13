#	$NetBSD: Makefile,v 1.4.40.1 2009/05/13 19:19:42 jym Exp $

.include <bsd.own.mk>

MODULE=		gettext

CONFIGURE_ARGS=	--program-transform-name="s,^,${_TOOL_PREFIX}," \
			--with-included-gettext --disable-csharp
MAKE_ARGS=	MAKEINFO=${TOOL_MAKEINFO:Q}

.include "${.CURDIR}/../Makefile.gnuhost"
