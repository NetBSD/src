#	$NetBSD: Makefile,v 1.9 2002/09/17 23:18:27 thorpej Exp $

PROG=		crunchgen
SRCS=		crunchgen.c crunched_skel.c
CLEANFILES+=	crunched_skel.c

crunched_skel.c: mkskel.sh crunched_main.c
	sh ${.ALLSRC} >${.TARGET}

.ifndef HOSTPROG
.include <bsd.prog.mk>

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
