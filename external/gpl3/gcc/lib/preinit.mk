# $NetBSD: preinit.mk,v 1.3.4.2 2025/08/02 05:25:55 perseant Exp $

FILE=lib${LIB}_preinit.o
FILES+=${FILE}
FILESDIR=${LIBDIR}
FILESBUILD+=${FILE}
CLEANFILES+=${FILE} ${LIB}_preinit.o

${FILE}: ${LIB}_preinit.o
	${_MKTARGET_CREATE}
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.files.mk>
