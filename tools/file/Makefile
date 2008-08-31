#	$NetBSD: Makefile,v 1.5 2008/08/31 00:31:22 lukem Exp $
#

GNUHOSTDIST=	${.CURDIR}/../../dist/file

CONFIGURE_ENV+=	CC=${HOST_CC:Q} \
		CFLAGS=${HOST_CFLAGS:Q} \
		CPPFLAGS=${HOST_CPPFLAGS:Q} \
		CXX=${HOST_CXX:Q}

CONFIGURE_ARGS=	--program-prefix=${_TOOL_PREFIX} --disable-shared \
		--disable-warnings

.NOTPARALLEL:

.include "${.CURDIR}/../Makefile.gnuhost"
