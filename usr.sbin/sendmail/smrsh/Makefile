#	@(#)Makefile	8.1 (Berkeley) 7/2/95

BINDIR=	/usr/libexec
PROG=	smrsh
MAN=	smrsh.8
CPPFLAGS+=-I${.CURDIR}/../src -DNDBM -DNEWDB
CPPFLAGS+=-DCMDDIR=\"/usr/libexec/sm.bin\"

.include <bsd.prog.mk>
