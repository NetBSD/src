# $NetBSD: preinit.mk,v 1.2 2025/03/23 18:56:43 christos Exp $

FILE=lib${LIB}_preinit.o
FILES+=${FILE}
FILESDIR=${LIBDIR}
CLEANFILES+=${FILE} ${LIB}_preinit.o

${FILE}: ${LIB}_preinit.o
	${_MKTARGET_CREATE}
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.files.mk>
