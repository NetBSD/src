#	$NetBSD: Makefile,v 1.1.2.1 2004/07/12 21:36:18 he Exp $

.include <bsd.own.mk>

MODULE=		gettext

CONFIGURE_ARGS=	--program-transform-name="s,^,nb," \
		--with-included-gettext
MAKE_ARGS=	MAKEINFO=${MAKEINFO:Q}

.include "${.CURDIR}/../Makefile.gnuhost"
