#	$NetBSD: Makefile,v 1.6 2009/05/08 17:45:22 christos Exp $
#

GNUHOSTDIST=	${.CURDIR}/../../external/bsd/file/dist

CONFIGURE_ENV+=	CC=${HOST_CC:Q} \
		CFLAGS=${HOST_CFLAGS:Q} \
		CPPFLAGS=${HOST_CPPFLAGS:Q} \
		CXX=${HOST_CXX:Q}

CONFIGURE_ARGS=	--program-prefix=${_TOOL_PREFIX} --disable-shared \
		--disable-warnings

.NOTPARALLEL:

.include "${.CURDIR}/../Makefile.gnuhost"
