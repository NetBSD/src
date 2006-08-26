#	$NetBSD: Makefile,v 1.14 2006/08/26 18:17:42 christos Exp $

PROG=		crunchgen
SRCS=		crunchgen.c crunched_skel.c
CLEANFILES+=	crunched_skel.c

crunched_skel.c: mkskel.sh crunched_main.c
	${_MKTARGET_CREATE}
	${HOST_SH} ${.ALLSRC} >${.TARGET}


LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>

.ifndef HOSTPROG
# Native tool -- key only off TOOLCHAIN_MISSING
.if ${TOOLCHAIN_MISSING} == "no"
CPPFLAGS+=	-DNEW_TOOLCHAIN
.endif
.else
# Host tool -- We can also pay attention to EXTERNAL_TOOLCHAIN.
.if ${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN)
CPPFLAGS+=	-DNEW_TOOLCHAIN
.endif
.endif	# ! HOSTPROG
