#	$NetBSD: Makefile,v 1.4 1997/04/16 13:29:58 christos Exp $

PROG=	mopchk
SRCS=	mopchk.c device.c version.c pf.c loop-bsd.c file.c
CFLAGS+= -I${.CURDIR}/.. -DINFO
CLEANFILES= version.c version.h
LDADD=	-lkvm
DPADD=	${LIBKVM}
.PATH:	${.CURDIR}/../common 

version.c version.h: ${.CURDIR}/../common/VERSION
	rm -f version.c; \
	sed 's/.*/char version[] = "&";/' ${.ALLSRC} > version.c
	set `sed 's/\([0-9]*\)\.\([0-9]*\).*/\1 \2/' ${.ALLSRC}` ; \
		{ echo '#define VERSION_MAJOR' $$1 ; \
		  echo '#define VERSION_MINOR' $$2 ; } > version.h


.include <bsd.prog.mk>
