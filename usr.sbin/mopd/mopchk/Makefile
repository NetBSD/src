#	$NetBSD: Makefile,v 1.5 1997/04/17 21:09:09 christos Exp $

PROG=	mopchk
SRCS=	mopchk.c device.c version.c pf.c loop-bsd.c file.c
CFLAGS+= -DINFO
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
