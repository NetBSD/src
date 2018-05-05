#	$NetBSD: Makefile,v 1.6.22.1 2018/05/05 19:25:56 martin Exp $

.include <bsd.init.mk>

MODULE=		gettext

CONFIGURE_ARGS=	--program-transform-name="s,^,${_TOOL_PREFIX}," \
			--with-included-gettext --disable-csharp
CONFIGURE_ENV+=	EMACS=no
MAKE_ARGS=	MAKEINFO=${TOOL_MAKEINFO:Q}

.include "${.CURDIR}/../Makefile.gnuhost"
