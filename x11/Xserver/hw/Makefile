#	$NetBSD: Makefile,v 1.7 2004/01/06 12:30:13 mrg Exp $

.include "../Makefile.common"

SUBDIR= xnest

.if ${XSERVER_XVFB} != "no"
SUBDIR+=vfb
.endif

.if ${XSERVER_XSUN} != "no"
SUBDIR+=sun
.endif

.if ${XSERVER_XFREE86} != "no"
SUBDIR+=xfree86
.endif

.if ${XSERVER_XMACPPC} != "no"
SUBDIR+=netbsd/macppc
.endif

.if ${XSERVER_XHPC} != "no"
SUBDIR+=netbsd/hpc
.endif

.include <bsd.subdir.mk>
