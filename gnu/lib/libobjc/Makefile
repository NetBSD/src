#	$NetBSD: Makefile,v 1.21 2002/09/17 23:18:35 thorpej Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

LIB=		objc

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

# Machine-independent definitions (include file names).
.include "${.CURDIR}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/toolchain
GCCARCH=	${NETBSDSRCDIR}/gnu/usr.bin/gcc/arch/${MACHINE_ARCH}

SRCS=		${G_OBJS:N[A-Z]*:Nlinking.o:.o=.c} ${G_OBJS:M[A-Z]*:.o=.m} linking.m

GCPPFLAGS=	${G_ALL_CFLAGS} ${G_INCLUDES}
CPPFLAGS+=	-I. -I${GCCARCH} ${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*}

INCS=		${G_OBJC_H}
INCSDIR=	/usr/include/objc

DPSRCS+=	runtime-info.h
CLEANFILES+=	rtscratch rtscratch.s
runtime-info.h:
	@touch rtscratch
	`${OBJC} --print-prog-name=cc1obj` -print-objc-runtime-info rtscratch >$@

.PATH: ${DIST}/libobjc ${DIST}/libobjc/objc

.include <bsd.lib.mk>

${OBJS} ${POBJS} ${SOBJS}: runtime-info.h
