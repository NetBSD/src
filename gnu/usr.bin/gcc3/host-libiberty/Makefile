#	$NetBSD: Makefile,v 1.3 2003/10/20 01:15:03 lukem Exp $

libiberty:
	mkdir libiberty
	(cd libiberty; env CC=${HOST_CC:Q} ${DIST}/libiberty/configure && env CC=${HOST_CC:Q} ${MAKE})

cleandir:
	${_MKCMD}\
	rm -rf libiberty

depend dependall all: libiberty
	
.include <bsd.prog.mk>
