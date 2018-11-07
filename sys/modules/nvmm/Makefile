#	$NetBSD: Makefile,v 1.1 2018/11/07 07:43:08 maxv Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

CPPFLAGS+=

.PATH:	${S}/dev/nvmm
.PATH:	${S}/dev/nvmm/x86

KMOD=	nvmm
IOCONF=	nvmm.ioconf
SRCS=	nvmm.c

.if ${MACHINE_ARCH} == "x86_64"
SRCS+=	nvmm_x86_svm.c nvmm_x86_svmfunc.S
.endif

.include <bsd.kmodule.mk>
