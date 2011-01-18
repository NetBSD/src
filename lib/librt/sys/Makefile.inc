#	$NetBSD: Makefile.inc,v 1.9 2011/01/18 16:41:51 matt Exp $

.PATH: ${.CURDIR}/sys
.PATH: ${ARCHDIR}/sys

ASM=		aio_cancel.S aio_error.S aio_fsync.S aio_read.S aio_return.S \
		aio_write.S lio_listio.S \
		mq_close.S mq_getattr.S mq_notify.S mq_open.S \
		mq_setattr.S mq_unlink.S \
		pset_create.S pset_destroy.S pset_assign.S _pset_bind.S

SRCS+=		${ASM}
CLEANFILES+=	${ASM}

LIBCDIR!=	cd ${NETBSDSRCDIR}/lib/libc && ${PRINTOBJDIR}

SRCS+=		cerror.S
CPPFLAGS+=	-D__cerror=__rt_cerror -I${LIBCDIR} -D_REENTRANT

ASMDEPS=	${.CURDIR}/sys/Makefile.inc ${ARCHDIR}/SYS.h \
		${DESTDIR}/usr/include/sys/syscall.h

${ASM}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nRSYSCALL(${.PREFIX})\n' >${.TARGET}
