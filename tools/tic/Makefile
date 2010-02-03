#	$NetBSD: Makefile,v 1.1 2010/02/03 15:34:44 roy Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}tic
HOST_SRCDIR=	usr.bin/tic


.include "${.CURDIR}/../Makefile.host"

.PATH:		${NETBSDSRCDIR}/lib/libterminfo
SRCS+=		hash.c
CLEANFILES+=	hash.c
tic:		hash.lo

# Generate our string and hash tables
hash.c:
		@echo "Generating terminfo hash"
		cd ${NETBSDSRCDIR}/lib/libterminfo; ./genhash >${.OBJDIR}/$@
