#	$NetBSD: Makefile.inc,v 1.1.4.2 2002/07/14 17:46:41 gehenna Exp $

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
