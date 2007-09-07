#	$NetBSD: Makefile.inc,v 1.3 2007/09/07 18:56:05 rmind Exp $

.PATH: ${.CURDIR}/sys

ASM=		aio_cancel.S aio_error.S aio_fsync.S aio_read.S aio_return.S \
		aio_suspend.S aio_write.S lio_listio.S \
		mq_close.S mq_getattr.S mq_notify.S mq_open.S mq_receive.S \
		mq_send.S mq_setattr.S mq_timedreceive.S mq_timedsend.S \
		mq_unlink.S

SRCS+=		${ASM}
CLEANFILES+=	${ASM}

ASMDEPS=	${.CURDIR}/sys/Makefile.inc ${ARCHDIR}/SYS.h \
		${DESTDIR}/usr/include/sys/syscall.h

${ASM}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nRSYSCALL(${.PREFIX})\n' >${.TARGET}
