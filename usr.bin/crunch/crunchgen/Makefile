#	$NetBSD: Makefile,v 1.16 2019/10/13 07:28:15 mrg Exp $

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
