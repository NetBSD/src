#	$NetBSD: Makefile.inc,v 1.2.4.2 2019/06/10 22:10:16 christos Exp $

XZ_TOPLEVEL:=	${.PARSEDIR}/../../external/public-domain/xz

NOMAN=          # defined
MKREPRO=no	# Native toolchain might be unable to do it
MKNLS=no

CPPFLAGS+=	-std=c99

.include "${XZ_TOPLEVEL}/Makefile.inc"
