#	$NetBSD: Makefile.inc,v 1.1 2002/06/06 19:48:11 fredette Exp $

BINDIR=			/usr/mdec

release:

S?=		${.CURDIR}/../../../..

.if !(defined(PROG) && ${PROG} == "mkboot")

.BEGIN: machine hp700
.NOPATH: machine hp700
realall realdepend: machine hp700

machine hp700 :
	-rm -f $@
	ln -s ${S}/arch/hp700/include $@

CLEANFILES+=machine hp700

.endif
