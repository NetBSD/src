#	$NetBSD: Makefile,v 1.10 2003/05/18 07:57:33 lukem Exp $

PROG=		crunchgen
SRCS=		crunchgen.c crunched_skel.c
CLEANFILES+=	crunched_skel.c

crunched_skel.c: mkskel.sh crunched_main.c
	sh ${.ALLSRC} >${.TARGET}

.include <bsd.prog.mk>

.ifndef HOSTPROG
# Native tool -- key only off TOOLCHAIN_MISSING
.if ${TOOLCHAIN_MISSING} != "yes"
CPPFLAGS+=	-DNEW_TOOLCHAIN
.endif
.else
# Host tool -- We can also pay attention to EXTERNAL_TOOLCHAIN.
.if ${TOOLCHAIN_MISSING} != "yes" || defined(EXTERNAL_TOOLCHAIN)
CPPFLAGS+=	-DNEW_TOOLCHAIN
.endif
.endif	# ! HOSTPROG
