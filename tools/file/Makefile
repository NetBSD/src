#	$NetBSD: Makefile,v 1.3 2005/02/11 15:01:57 jmc Exp $
#

GNUHOSTDIST=	${.CURDIR}/../../dist/file

CONFIGURE_ENV+=	CC=${HOST_CC:Q} \
		CFLAGS=${HOST_CFLAGS:Q} \
		CPPFLAGS=${HOST_CPPFLAGS:Q} \
		CXX=${HOST_CXX:Q}

CONFIGURE_ARGS=	--program-prefix=${_TOOL_PREFIX}

.NOTPARALLEL:

.include "${.CURDIR}/../Makefile.gnuhost"
