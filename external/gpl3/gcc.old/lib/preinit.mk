# $NetBSD: preinit.mk,v 1.2 2025/09/07 04:11:47 mrg Exp $

FILE=lib${LIB}_preinit.o
FILES+=${FILE}
FILESDIR=${LIBDIR}
FILESBUILD+=${FILE}
CLEANFILES+=${FILE} ${LIB}_preinit.o

${FILE}: ${LIB}_preinit.o
	${_MKTARGET_CREATE}
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.files.mk>
