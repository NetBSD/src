#	$NetBSD: Makefile,v 1.1 2000/05/02 12:01:11 simonb Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
NTPSRCDIR=html/hints
FILESDIR=${NTPDOCDIR}/hints
FILES=	a-ux aix bsdi changes decosf1 decosf2 hpux linux notes-xntp-v3 parse \
	refclocks rs6000 sco.htm sgi solaris-dosynctodr.html solaris.html \
	solaris.xtra.4023118 solaris.xtra.4095849 solaris.xtra.S99ntpd \
	solaris.xtra.patchfreq sun4 svr4-dell svr4_package todo vxworks.html \
	winnt.htm
.endif
MKOBJ=  no

.include "../../Makefile.inc"
.include <bsd.prog.mk>
