#	$NetBSD: Makefile,v 1.11 2003/10/19 04:05:01 lukem Exp $

PROG=		crunchgen
SRCS=		crunchgen.c crunched_skel.c
CLEANFILES+=	crunched_skel.c

crunched_skel.c: mkskel.sh crunched_main.c
	${_MKMSGCREATE}
	${_MKCMD}\
	${HOST_SH} ${.ALLSRC} >${.TARGET}

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
