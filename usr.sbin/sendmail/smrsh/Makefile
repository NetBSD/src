#	@(#)Makefile	8.1 (Berkeley) 7/2/95

BINDIR=	/usr/libexec
PROG=	smrsh
MAN=	smrsh.8
CFLAGS+=-I${.CURDIR}/../src -DNDBM -DNEWDB
CFLAGS+=-DCMDDIR=/usr/libexec/sm.bin

.include "../../Makefile.inc"
.include <bsd.prog.mk>
