#	$NetBSD: Makefile,v 1.4 2002/12/08 20:19:57 thorpej Exp $

MODULE=		autoconf

CONFIGURE_ARGS=	--program-prefix=${_TOOL_PREFIX}
CONFIGURE_ENV=	M4="${TOOLDIR}/bin/${_TOOL_PREFIX}m4 -g -D__gnu__"

.include "${.CURDIR}/../Makefile.gnuhost"
