#	$NetBSD: Makefile,v 1.8 2023/06/03 09:09:10 lukem Exp $
#

NOFULLRELRO=	yes

RUMPTOP=	${.CURDIR}/../../sys/rump

LIBDPLIBS+=     rumpuser ${.CURDIR}/../librumpuser

WARNS=		3	# XXX: kernel isn't ready for -Wsign-compare

COPTS.kern_hook.c+=	${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.rump.c+=		${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.subr_pool.c+=	${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.subr_vmem.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include "${RUMPTOP}/librump/rumpkern/Makefile.rumpkern"
