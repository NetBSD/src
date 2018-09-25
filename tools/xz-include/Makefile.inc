#	$NetBSD: Makefile.inc,v 1.2 2018/09/25 17:57:33 riz Exp $

XZ_TOPLEVEL:=	${.PARSEDIR}/../../external/public-domain/xz

NOMAN=          # defined
MKREPRO=no	# Native toolchain might be unable to do it
MKNLS=no

CPPFLAGS+=	-std=c99

.include "${XZ_TOPLEVEL}/Makefile.inc"
