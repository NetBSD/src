# $NetBSD: preinit.mk,v 1.1 2025/03/22 17:05:48 christos Exp $

FILE=lib${LIB}_preinit.o
FILES+=${FILE}
FILESDIR=/usr/lib
CLEANFILES+=${FILE} ${LIB}_preinit.o

${FILE}: ${LIB}_preinit.o
	${_MKTARGET_CREATE}
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.files.mk>
