#	$NetBSD: Makefile,v 1.15.82.1 2020/04/13 08:05:41 martin Exp $

PROG=		crunchgen
SRCS=		crunchgen.c crunched_skel.c
CLEANFILES+=	crunched_skel.c

crunched_skel.c: mkskel.sh crunched_main.c
	${_MKTARGET_CREATE}
	${HOST_SH} ${.ALLSRC} >${.TARGET}


.include <bsd.prog.mk>

.ifndef HOSTPROG

LDADD+=-lutil
DPADD+=${LIBUTIL}

# Native tool -- key only off TOOLCHAIN_MISSING
.if ${TOOLCHAIN_MISSING} == "no"
CPPFLAGS+=	-DNEW_TOOLCHAIN
.endif

COPTS.crunchgen.c+=	${GCC_NO_FORMAT_TRUNCATION}

.else

# Host tool -- We can also pay attention to EXTERNAL_TOOLCHAIN.
.if ${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN)
CPPFLAGS+=	-DNEW_TOOLCHAIN
.endif

.endif	# ! HOSTPROG
