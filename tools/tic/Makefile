#	$NetBSD: Makefile,v 1.5.40.1 2020/04/08 14:09:14 martin Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}tic
HOST_SRCDIR=	usr.bin/tic

# We need our libterminfo to compile compat and write to the database
CPPFLAGS+=	-DTERMINFO_COMPILE -DTERMINFO_COMPAT -DTERMINFO_DB
HOST_SRCS=	compile.c hash.c

TERMINFODIR=	${NETBSDSRCDIR}/lib/libterminfo

.include "${.CURDIR}/../Makefile.host"
.include "${TERMINFODIR}/Makefile.hash"

.PATH:		${TERMINFODIR}
