# $NetBSD: Makefile.inc,v 1.5 2005/12/27 08:43:05 yamt Exp $

COMMON_DIR:=${.PARSEDIR}
COMMON_CODEDIRS=gen gmon hash/sha1 inet md net quad stdlib string

.if defined(LIBKERN_ARCH) && !empty(LIBKERN_ARCH) && \
    exists(${KERNDIR}/arch/${LIBKERN_ARCH})
COMMON_ARCHSUBDIR=	${LIBKERN_ARCH}
.elif exists(${COMMON_DIR}/arch/${MACHINE_ARCH})
COMMON_ARCHSUBDIR=	${MACHINE_ARCH}
.elif exists(${COMMON_DIR}/arch/${MACHINE_CPU})
COMMON_ARCHSUBDIR=	${MACHINE_CPU}
.endif

COMMON_ARCHDIR=${COMMON_DIR}/arch/${COMMON_ARCHSUBDIR}

.for i in ${COMMON_CODEDIRS}
.if exists(${COMMON_DIR}/$i)
.PATH.c: ${COMMON_DIR}/$i
.endif
.if exists(${COMMON_ARCHDIR}/$i)
.PATH.S: ${COMMON_ARCHDIR}/$i
.endif
.endfor

CPPFLAGS+=-I${COMMON_DIR}/quad -I${COMMON_DIR}/string
.if defined(COMMON_ARCHSUBDIR)
CPPFLAGS+=-I${COMMON_ARCHDIR}/string
.endif
