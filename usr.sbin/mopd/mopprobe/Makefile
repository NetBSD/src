#	$NetBSD: Makefile,v 1.5 1997/04/17 21:09:25 christos Exp $

PROG=	mopprobe
SRCS=	mopprobe.c device.c get.c cmp.c mopdef.c version.c pf.c loop-bsd.c
CFLAGS+= -DNODL
CLEANFILES= version.c version.h
LDADD=	-lkvm
DPADD=	${LIBKVM}

version.c version.h: ${.CURDIR}/../common/VERSION
	rm -f version.c; \
	sed 's/.*/char version[] = "&";/' ${.ALLSRC} > version.c
	set `sed 's/\([0-9]*\)\.\([0-9]*\).*/\1 \2/' ${.ALLSRC}` ; \
		{ echo '#define VERSION_MAJOR' $$1 ; \
		  echo '#define VERSION_MINOR' $$2 ; } > version.h

.include <bsd.prog.mk>
