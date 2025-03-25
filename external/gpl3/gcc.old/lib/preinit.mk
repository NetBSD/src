# $NetBSD: preinit.mk,v 1.1 2025/03/25 18:03:32 christos Exp $

FILE=lib${LIB}_preinit.o
FILES+=${FILE}
FILESDIR=${LIBDIR}
FILESBUILD+=${FILE}
CLEANFILES+=${FILE} ${LIB}_preinit.o

${FILE}: ${LIB}_preinit.o
	${_MKTARGET_CREATE}
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.files.mk>
