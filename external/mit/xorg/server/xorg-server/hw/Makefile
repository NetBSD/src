#	$NetBSD: Makefile,v 1.5.28.1 2020/08/27 09:05:37 martin Exp $

.include "../Makefile.common"

SUBDIR= xnest vfb
.if ${XSERVER_XFREE86} != "no"
SUBDIR+= xfree86
.endif

.if ${XSERVER_X68K} != "no"
SUBDIR+=netbsd/x68k
.endif

.if ${XSERVER_XSUN} != "no" || \
    ${XSERVER_XSUNMONO} != "no" || \
    ${XSERVER_XSUN24} != "no"
SUBDIR+=sun
.endif

.if 0
.if ${XSERVER_XALPHANETBSD} != "no"
SUBDIR+=netbsd/alpha
.endif

.if ${XSERVER_XAMIGA} != "no"
SUBDIR+=netbsd/amiga
.endif

.if ${XSERVER_XARM32VIDC} != "no"
SUBDIR+=netbsd/arm32vidc
.endif

.if ${XSERVER_XDECNETBSD} != "no"
SUBDIR+=netbsd/dec
.endif

.if ${XSERVER_XDREAMCAST} != "no"
SUBDIR+=netbsd/dreamcast
.endif

.if ${XSERVER_XFREE86} != "no"
SUBDIR+=xfree86
.endif

.if ${XSERVER_XHPC} != "no"
SUBDIR+=netbsd/hpc
.endif

.if ${XSERVER_XMAC68K} != "no"
SUBDIR+=netbsd/mac68k
.endif

.if ${XSERVER_XMACPPC} != "no"
SUBDIR+=netbsd/macppc
.endif

.if ${XSERVER_XNEWSMIPS} != "no"
SUBDIR+=netbsd/newsmips
.endif

.if ${XSERVER_XVFB} != "no"
SUBDIR+=vfb
.endif
.endif

.include <bsd.subdir.mk>
