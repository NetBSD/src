#	$NetBSD: Makefile.inc,v 1.2 2002/11/28 06:01:00 chs Exp $

BINDIR=			/usr/mdec

release:

S?=		${.CURDIR}/../../../..

.if !(defined(PROG) && ${PROG} == "mkboot")

.BEGIN: machine hp700 hppa
.NOPATH: machine hp700 hppa
realall realdepend: machine hp700 hppa

machine hp700 :
	-rm -f $@
	ln -s ${S}/arch/hp700/include $@

hppa :
	-rm -f $@
	ln -s ${S}/arch/hppa/include $@

CLEANFILES+=machine hp700 hppa

.endif
