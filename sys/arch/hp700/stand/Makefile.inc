#	$NetBSD: Makefile.inc,v 1.1.2.2 2002/06/23 17:36:29 jdolecek Exp $

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
