#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:38 mrg Exp $

libiberty:
	mkdir libiberty
	(cd libiberty; env CC=${HOST_CC:Q} ${DIST}/libiberty/configure && env CC=${HOST_CC:Q} ${MAKE})

cleandir:
	rm -rf libiberty

dependall all: libiberty
	
.include <bsd.prog.mk>
