#	$NetBSD: Makefile.inc,v 1.5 2008/01/15 03:37:15 rmind Exp $

.PATH: ${.CURDIR}/sys

ASM=		aio_cancel.S aio_error.S aio_fsync.S aio_read.S aio_return.S \
		aio_write.S lio_listio.S \
		mq_close.S mq_getattr.S mq_notify.S mq_open.S \
		mq_setattr.S mq_unlink.S \
		pset_create.S pset_destroy.S pset_assign.S _pset_bind.S

SRCS+=		${ASM}
CLEANFILES+=	${ASM}

ASMDEPS=	${.CURDIR}/sys/Makefile.inc ${ARCHDIR}/SYS.h \
		${DESTDIR}/usr/include/sys/syscall.h

${ASM}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nRSYSCALL(${.PREFIX})\n' >${.TARGET}
