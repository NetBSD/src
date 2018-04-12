# $NetBSD: Makefile.inc,v 1.2.8.1 2018/04/12 01:38:59 msaitoh Exp $

.include "../Makefile.inc"

BINDIR=/usr/sbin

CPPFLAGS+=-I${UNBOUND} -I${UNBOUND}/../include

PROGDPLIBS+= unbound ${UNBOUND}/../lib/libunbound
PROGDPLIBS+= ${DPLIBS}

.if defined(PROG)
.if ${PROG} != "unbound"
.PATH: ${UNBOUND}/smallapp
SRCS+=${PROG}.c
.else
MAN+=${PROG}.conf.5
${PROG}.conf.5: ${PROG}.conf.5.in __subst
CLEANFILES+=${PROG}.conf.5
.PATH: ${UNBOUND}/daemon ${UNBOUND}/util/shm_side
SRCS+= acl_list.c cachedump.c daemon.c remote.c stats.c unbound.c worker.c
SRCS+= shm_main.c
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
