#	$NetBSD: Makefile,v 1.5.12.1 2013/06/23 06:28:58 tls Exp $

.include <bsd.own.mk>

MODULE=		gettext

CONFIGURE_ARGS=	--program-transform-name="s,^,${_TOOL_PREFIX}," \
			--with-included-gettext --disable-csharp
CONFIGURE_ENV+=	EMACS=no
MAKE_ARGS=	MAKEINFO=${TOOL_MAKEINFO:Q}

.include "${.CURDIR}/../Makefile.gnuhost"
