#	$NetBSD: Makefile.inc,v 1.6 2005/12/11 12:18:23 christos Exp $

BINDIR= /usr/mdec

CFLAGS+= -ffreestanding

# if there is a 'version' file, add rule for vers.c and add it to SRCS
# and CLEANFILES
.if exists(version)
.PHONY: vers.c
vers.c: version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.CURDIR}/version ${MACHINE}

SRCS+=	vers.c
CLEANFILES+= vers.c
.endif

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=
