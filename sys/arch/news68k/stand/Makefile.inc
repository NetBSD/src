#	$NetBSD: Makefile.inc,v 1.3 2003/10/08 01:40:22 simonb Exp $

BINDIR= /usr/mdec

# if there is a 'version' file, add rule for vers.c and add it to SRCS
# and CLEANFILES
.if exists(version)
.PHONY: vers.c
vers.c: version
	sh ${S}/conf/newvers_stand.sh ${.CURDIR}/version ${MACHINE}

SRCS+=	vers.c
CLEANFILES+= vers.c
.endif

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=
