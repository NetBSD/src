#	$NetBSD: Makefile.inc,v 1.2.6.1 2004/08/03 10:34:54 skrll Exp $

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
