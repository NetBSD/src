#	$NetBSD: Makefile,v 1.1 2002/03/28 22:03:04 thorpej Exp $

.include "../../../../Makefile.inc"
.include <bsd.own.mk>

KERNEL?= ${KERNOBJDIR}/IQ80321/netbsd.bin

all:
	cp ${KERNEL} netbsd.bin
	gzip -9f netbsd.bin

clean cleandir distclean:
	rm -f netbsd.bin netbsd.bin.gz

# nothing to do here
depend includes lint regress tags:

.include <bsd.kernobj.mk>
.include <bsd.obj.mk>
