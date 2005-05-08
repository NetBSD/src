#	$NetBSD: Makefile,v 1.4 2005/05/08 01:47:33 chs Exp $
#

GNUHOSTDIST=	${.CURDIR}/../../dist/file

CONFIGURE_ENV+=	CC=${HOST_CC:Q} \
		CFLAGS=${HOST_CFLAGS:Q} \
		CPPFLAGS=${HOST_CPPFLAGS:Q} \
		CXX=${HOST_CXX:Q}

CONFIGURE_ARGS=	--program-prefix=${_TOOL_PREFIX} --disable-shared

.NOTPARALLEL:

.include "${.CURDIR}/../Makefile.gnuhost"
