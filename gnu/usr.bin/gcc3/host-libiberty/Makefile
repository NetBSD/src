#	$NetBSD: Makefile,v 1.4 2003/10/21 10:01:20 lukem Exp $

libiberty:
	mkdir libiberty
	(cd libiberty; env CC=${HOST_CC:Q} ${DIST}/libiberty/configure && env CC=${HOST_CC:Q} ${MAKE})

cleandir:
	rm -rf libiberty

depend dependall all: libiberty
	
.include <bsd.prog.mk>
