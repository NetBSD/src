# $NetBSD: Makefile.inc,v 1.1 2016/08/20 11:20:32 christos Exp $

.include "../Makefile.inc"

BINDIR=/usr/sbin

CPPFLAGS+=-I${UNBOUND} -I${UNBOUND}/../include

PROGDPLIBS+= unbound ${UNBOUND}/../lib/libunbound
PROGDPLIBS+= ${DPLIBS}

.if defined(PROG)
.if ${PROG} != "unbound"
.PATH: ${UNBOUND}/smallapp
SRCS+=${PROG}.c worker_cb.c
.else
MAN+=${PROG}.conf.5
${PROG}.conf.5: ${PROG}.conf.5.in __subst
.PATH: ${UNBOUND}/daemon
SRCS+= acl_list.c cachedump.c daemon.c remote.c stats.c unbound.c worker.c
.endif
CLEANFILES+=${PROG}.8
MAN+=${PROG}.8
${PROG}.8: ${PROG}.8.in __subst
.endif

.if defined(SCRIPTS)
.PATH: ${UNBOUND}/smallapp
CLEANFILES+=${SCRIPTS}.sh
${SCRIPTS}.sh: ${SCRIPTS}.sh.in __subst
.endif
