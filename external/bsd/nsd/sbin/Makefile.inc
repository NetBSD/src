# $NetBSD: Makefile.inc,v 1.2 2019/03/29 14:38:30 christos Exp $

.include "../Makefile.inc"

BINDIR=/usr/sbin

PROGDPLIBS+= nsd ${NSD}/../lib/libnsd
PROGDPLIBS+= ${DPLIBS}

.if defined(PROG)

.if ${PROG} == "nsd" || ${PROG} == "nsd-checkzone"
PROGDPLIBS+= xfrd ${NSD}/../lib/libxfrd
PROGDPLIBS+= ${DPLIBS}
.endif

CLEANFILES+=${PROG}.8
MAN+=${PROG}.8
${PROG}.8: ${PROG}.8.in __subst

.endif


.if defined(SCRIPTS)
CLEANFILES+=${SCRIPTS}.sh
${SCRIPTS}.sh: ${SCRIPTS}.sh.in __subst
.endif
