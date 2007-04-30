#	$NetBSD: Makefile.inc,v 1.1 2007/04/30 14:44:29 rmind Exp $

.PATH: ${.CURDIR}/sys

ASM=		aio_cancel.S aio_error.S aio_fsync.S aio_read.S aio_return.S \
		aio_suspend.S aio_write.S lio_listio.S

SRCS+=		${ASM}
CLEANFILES+=	${PSEUDO}

ASMDEPS=	${.CURDIR}/sys/Makefile.inc ${ARCHDIR}/SYS.h \
		${DESTDIR}/usr/include/sys/syscall.h

${ASM}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nRSYSCALL(${.PREFIX})\n' >${.TARGET}
