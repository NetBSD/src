#	$NetBSD: Makefile,v 1.1 2006/03/29 21:10:26 jmc Exp $
#

GNUHOSTDIST=	${.CURDIR}/../../gnu/dist/gmake

CONFIGURE_ENV+=	CC=${HOST_CC:Q} \
		CFLAGS=${HOST_CFLAGS:Q} \
		CPPFLAGS=${HOST_CPPFLAGS:Q} \
		CXX=${HOST_CXX:Q}

CONFIGURE_ARGS=	--program-prefix=${_TOOL_PREFIX}g

.NOTPARALLEL:

.include "${.CURDIR}/../Makefile.gnuhost"
