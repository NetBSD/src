#	$NetBSD: Makefile,v 1.1 2006/05/10 22:53:50 mrg Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

LIB=		gcc_eh

.include <bsd.own.mk>

.if ${MKGCC} != "no"
.if exists(${.CURDIR}/../arch/${MACHINE_ARCH}.mk)
.include "${.CURDIR}/../arch/${MACHINE_ARCH}.mk"
.endif

.cc: # disable .cc->NULL transform

SRCS+=		${LIB2_EH}

.include <bsd.lib.mk>

${LIB2_EH}: unwind.h
unwind.h: ${G_UNWIND_H}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${G_UNWIND_H} ${.TARGET}
CLEANFILES+= unwind.h

.else
.include <bsd.prog.mk> # do nothing
.endif
