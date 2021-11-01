#	$NetBSD: Makefile,v 1.7 2021/11/01 21:37:32 nia Exp $
#

NOFULLRELRO=	yes

RUMPTOP=	${.CURDIR}/../../sys/rump

LIBDPLIBS+=     rumpuser ${.CURDIR}/../librumpuser

WARNS=		3	# XXX: kernel isn't ready for -Wsign-compare

COPTS.kern_hook.c+=	${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.rump.c+=		${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.subr_pool.c+=	${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.subr_vmem.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include "${RUMPTOP}/librump/rumpkern/Makefile.rumpkern"
