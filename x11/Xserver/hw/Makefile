#	$NetBSD: Makefile,v 1.9 2004/01/10 04:34:33 fredb Exp $

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

.if ${XSERVER_X68K} != "no"
SUBDIR+=netbsd/x68k
.endif

.if ${XSERVER_XMAC68K} != "no"
SUBDIR+=netbsd/mac68k
.endif

.include <bsd.subdir.mk>
