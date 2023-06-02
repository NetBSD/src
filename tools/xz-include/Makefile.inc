#	$NetBSD: Makefile.inc,v 1.3 2023/06/02 20:47:27 lukem Exp $

XZ_TOPLEVEL:=	${.PARSEDIR}/../../external/public-domain/xz

NOMAN=		# defined
NONLS=		# defined
MKREPRO=no	# Native toolchain might be unable to do it

CPPFLAGS+=	-std=c99

.include "${XZ_TOPLEVEL}/Makefile.inc"
