#	$NetBSD: Makefile,v 1.6 2019/10/13 07:28:10 mrg Exp $
#

RUMPTOP=	${.CURDIR}/../../sys/rump

LIBDPLIBS+=     rumpuser ${.CURDIR}/../librumpuser

WARNS=		3	# XXX: kernel isn't ready for -Wsign-compare

COPTS.kern_hook.c+=	${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.rump.c+=		${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.subr_pool.c+=	${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.subr_vmem.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include "${RUMPTOP}/librump/rumpkern/Makefile.rumpkern"
