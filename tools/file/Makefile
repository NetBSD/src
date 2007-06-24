#	$NetBSD: Makefile,v 1.3.2.1 2007/06/24 19:42:10 ghen Exp $
#

GNUHOSTDIST=	${.CURDIR}/../../dist/file

CONFIGURE_ENV+=	CC=${HOST_CC:Q} \
		CFLAGS=${HOST_CFLAGS:Q} \
		CPPFLAGS=${HOST_CPPFLAGS:Q} \
		CXX=${HOST_CXX:Q}

CONFIGURE_ARGS=	--program-prefix=${_TOOL_PREFIX} --disable-shared

.NOTPARALLEL:

.include "${.CURDIR}/../Makefile.gnuhost"
