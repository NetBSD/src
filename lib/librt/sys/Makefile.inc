#	$NetBSD: Makefile.inc,v 1.7 2011/01/16 03:29:36 matt Exp $

.PATH: ${.CURDIR}/sys
.PATH: ${ARCHDIR}/sys

ASM=		aio_cancel.S aio_error.S aio_fsync.S aio_read.S aio_return.S \
		aio_write.S lio_listio.S \
		mq_close.S mq_getattr.S mq_notify.S mq_open.S \
		mq_setattr.S mq_unlink.S \
		pset_create.S pset_destroy.S pset_assign.S _pset_bind.S

SRCS+=		${ASM}
CLEANFILES+=	${ASM}

SRCS+=		cerror.S
CPPFLAGS+=	-D__cerror=__rt_cerror -I../libc

ASMDEPS=	${.CURDIR}/sys/Makefile.inc ${ARCHDIR}/SYS.h \
		${DESTDIR}/usr/include/sys/syscall.h

${ASM}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nRSYSCALL(${.PREFIX})\n' >${.TARGET}
