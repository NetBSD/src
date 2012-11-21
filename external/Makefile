#	$NetBSD: Makefile,v 1.16.6.1.2.1 2012/11/21 01:04:33 matt Exp $

.include <bsd.own.mk>

SUBDIR+= apache2 atheros bsd intel-fw-eula intel-fw-public gpl2
.if ${HAVE_BINUTILS} == "219" || ${HAVE_GCC} == "45" || ${HAVE_GDB} == "7"
SUBDIR+= gpl3
.endif
SUBDIR+= historical ibm-public public-domain cddl mit realtek

.include <bsd.subdir.mk>
