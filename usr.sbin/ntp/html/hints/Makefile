#	$NetBSD: Makefile,v 1.4 2003/04/18 18:33:23 thorpej Exp $

NOOBJ=  # defined

.include <bsd.own.mk>

.if ${MKDOC} != "no"
NTPSRCDIR=html/hints
FILESDIR=${NTPDOCDIR}/hints
FILES=	a-ux aix bsdi changes decosf1 decosf2 hpux linux notes-xntp-v3 parse \
	refclocks rs6000 sco.htm sgi solaris-dosynctodr.html solaris.html \
	solaris.xtra.4023118 solaris.xtra.4095849 solaris.xtra.S99ntpd \
	solaris.xtra.patchfreq sun4 svr4-dell svr4_package todo vxworks.html \
	winnt.htm
.endif

.include "../../Makefile.inc"
.include <bsd.prog.mk>
