# $NetBSD: Makefile.inc,v 1.2 2016/08/25 07:13:39 christos Exp $

.include "../Makefile.inc"

BINDIR=/usr/bin

CPPFLAGS+=-I${UNBOUND} -I${UNBOUND}/../include

PROGDPLIBS+= unbound ${UNBOUND}/../lib/libunbound
PROGDPLIBS+= ${DPLIBS}

.if defined(PROG)
.PATH: ${UNBOUND}/smallapp
SRCS+=${PROG}.c
CLEANFILES+=${PROG}.1
MAN+=${PROG}.1
${PROG}.1: ${PROG}.1.in __subst
.endif
