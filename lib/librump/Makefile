#	$NetBSD: Makefile,v 1.5.16.1 2020/04/13 08:03:16 martin Exp $
#

RUMPTOP=	${.CURDIR}/../../sys/rump

LIBDPLIBS+=     rumpuser ${.CURDIR}/../librumpuser

WARNS=		3	# XXX: kernel isn't ready for -Wsign-compare

COPTS.kern_hook.c+=	${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.rump.c+=		${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.subr_pool.c+=	${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.subr_vmem.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include "${RUMPTOP}/librump/rumpkern/Makefile.rumpkern"
