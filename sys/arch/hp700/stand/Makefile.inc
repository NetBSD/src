#	$NetBSD: Makefile.inc,v 1.3 2004/07/28 09:17:31 skrll Exp $

BINDIR=			/usr/mdec

release:

S?=		${.CURDIR}/../../../..

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
