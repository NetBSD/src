#	$NetBSD: Makefile,v 1.2 2003/10/06 21:40:32 veego Exp $

libiberty:
	mkdir libiberty
	(cd libiberty; env CC=${HOST_CC:Q} ${DIST}/libiberty/configure && env CC=${HOST_CC:Q} ${MAKE})

cleandir:
	rm -rf libiberty

depend dependall all: libiberty
	
.include <bsd.prog.mk>
