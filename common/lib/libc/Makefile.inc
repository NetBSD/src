# $NetBSD: Makefile.inc,v 1.1 2005/12/20 19:28:49 christos Exp $

COMMON_DIR:=${.PARSEDIR}
COMMON_CODEDIRS=gen gmon hash/sha1 inet md net quad stdlib string

.if exists(${COMMON_DIR}/arch/${MACHINE_ARCH})
COMMON_ARCHSUBDIR=     ${MACHINE_ARCH}
.elif exists(${COMMON_DIR}/arch/${MACHINE_CPU})
COMMON_ARCHSUBDIR=     ${MACHINE_CPU}
.else
.BEGIN:
        @echo no COMMON_ARCHSUBDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
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

CPPFLAGS+=-I${COMMON_DIR}/quad -I${COMMON_ARCHDIR}/string
