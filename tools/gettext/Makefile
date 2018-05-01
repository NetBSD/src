#	$NetBSD: Makefile,v 1.8 2018/05/01 19:59:47 christos Exp $

.include <bsd.hostinit.mk>

MODULE=		gettext

CONFIGURE_ARGS=	--program-transform-name="s,^,${_TOOL_PREFIX}," \
			--with-included-gettext --disable-csharp
CONFIGURE_ENV+=	EMACS=no
MAKE_ARGS=	MAKEINFO=${TOOL_MAKEINFO:Q}

.include "${.CURDIR}/../Makefile.gnuhost"
