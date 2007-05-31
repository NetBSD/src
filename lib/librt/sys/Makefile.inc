#	$NetBSD: Makefile.inc,v 1.2 2007/05/31 05:44:14 rmind Exp $

.PATH: ${.CURDIR}/sys

ASM=		aio_cancel.S aio_error.S aio_fsync.S aio_read.S aio_return.S \
		aio_suspend.S aio_write.S lio_listio.S

SRCS+=		${ASM}
CLEANFILES+=	${ASM}

ASMDEPS=	${.CURDIR}/sys/Makefile.inc ${ARCHDIR}/SYS.h \
		${DESTDIR}/usr/include/sys/syscall.h

${ASM}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nRSYSCALL(${.PREFIX})\n' >${.TARGET}
