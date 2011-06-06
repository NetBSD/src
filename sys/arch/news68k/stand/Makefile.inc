#	$NetBSD: Makefile.inc,v 1.8.22.1 2011/06/06 09:06:19 jruoho Exp $

BINDIR= /usr/mdec

CFLAGS+= -ffreestanding

# if there is a 'version' file, add rule for vers.c and add it to SRCS
# and CLEANFILES
.if exists(version)
.PHONY: vers.c
vers.c: version
	${_MKTARGET_CREATE}
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.CURDIR}/version ${MACHINE}

SRCS+=	vers.c
CLEANFILES+= vers.c
.endif

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=
