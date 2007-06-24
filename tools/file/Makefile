#	$NetBSD: Makefile,v 1.3.6.1 2007/06/24 19:45:41 ghen Exp $
#

GNUHOSTDIST=	${.CURDIR}/../../dist/file

CONFIGURE_ENV+=	CC=${HOST_CC:Q} \
		CFLAGS=${HOST_CFLAGS:Q} \
		CPPFLAGS=${HOST_CPPFLAGS:Q} \
		CXX=${HOST_CXX:Q}

CONFIGURE_ARGS=	--program-prefix=${_TOOL_PREFIX} --disable-shared

.NOTPARALLEL:

.include "${.CURDIR}/../Makefile.gnuhost"
