# $NetBSD: Makefile.inc,v 1.1 2016/08/20 11:20:31 christos Exp $

.include "../Makefile.inc"

BINDIR=/usr/bin

CPPFLAGS+=-I${UNBOUND} -I${UNBOUND}/../include

PROGDPLIBS+= unbound ${UNBOUND}/../lib/libunbound
PROGDPLIBS+= ${DPLIBS}

.if defined(PROG)
.PATH: ${UNBOUND}/smallapp
SRCS+=${PROG}.c worker_cb.c
CLEANFILES+=${PROG}.1
MAN+=${PROG}.1
${PROG}.1: ${PROG}.1.in __subst
.endif
