#	$NetBSD: Makefile,v 1.5 2001/10/05 22:52:56 jmc Exp $

PROG=crunchgen
SRCS=crunchgen.c crunched_skel.c
CLEANFILES+= crunched_skel.c

# XXX This should go away and the code cleaned up with all ports are converted
.if defined(USE_NEW_TOOLCHAIN)
CPPFLAGS= -DNEW_TOOLCHAIN
.endif

crunched_skel.c: crunched_main.c
	sh ${.CURDIR}/mkskel.sh ${.CURDIR}/crunched_main.c > crunched_skel.c

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
