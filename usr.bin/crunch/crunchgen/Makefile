#	$NetBSD: Makefile,v 1.7 2001/11/12 18:53:40 tv Exp $

PROG=		crunchgen
SRCS=		crunchgen.c crunched_skel.c
CLEANFILES+=	crunched_skel.c

# XXX This should go away and the code cleaned up with all ports are converted
.if defined(USE_NEW_TOOLCHAIN)
CPPFLAGS+=	-DNEW_TOOLCHAIN
.endif

crunched_skel.c: mkskel.sh crunched_main.c
	sh ${.ALLSRC} >${.TARGET}

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
