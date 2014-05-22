#	$NetBSD: Makefile,v 1.5.6.1 2014/05/22 11:42:30 yamt Exp $

.include <bsd.own.mk>

MODULE=		gettext

CONFIGURE_ARGS=	--program-transform-name="s,^,${_TOOL_PREFIX}," \
			--with-included-gettext --disable-csharp
CONFIGURE_ENV+=	EMACS=no
MAKE_ARGS=	MAKEINFO=${TOOL_MAKEINFO:Q}

.include "${.CURDIR}/../Makefile.gnuhost"
