#	$NetBSD: Makefile,v 1.6.28.1 2018/05/02 07:20:26 pgoyette Exp $

.include <bsd.hostinit.mk>

MODULE=		gettext

CONFIGURE_ARGS=	--program-transform-name="s,^,${_TOOL_PREFIX}," \
			--with-included-gettext --disable-csharp
CONFIGURE_ENV+=	EMACS=no
MAKE_ARGS=	MAKEINFO=${TOOL_MAKEINFO:Q}

.include "${.CURDIR}/../Makefile.gnuhost"
