#	$Id: Makefile.inc,v 1.3.2.2 1993/12/05 01:52:48 cgd Exp $

SRCS+=  __main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bcmp.c bzero.S ffs.S strcat.c strcmp.c strcpy.c strlen.S strncmp.c \
	strncpy.c htonl.S htons.S ntohl.S ntohs.S

SRCS+=	umul.S mul.S rem.S sdiv.S udiv.S umul.S urem.S
SRCS+=	_setjmp.S fabs.S fixunsdfsi.S modf.S mul.S saveregs.S setjmp.S umul.S
SRCS+=	frexp.c isinf.c isnan.c ldexp.c

# `source' files built from m4 source
# the name `div.o' is taken for the ANSI C `div' function, hence sdiv here
SRCS+=	rem.S sdiv.S udiv.S urem.S
CLEANFILES+=rem.S sdiv.S udiv.S urem.S

## Temporary hack for Theo's cross compile environment
CFLAGS+=-I/tmp/usr/include

sdiv.S: ${.CURDIR}/../../lib/libc/sparc/gen/divrem.m4
	echo 'building ${.TARGET} from ${.ALLSRC}'
	(echo "define(NAME,\`.div')define(OP,\`div')define(S,\`true')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	chmod 444 ${.TARGET}

udiv.S: ${.CURDIR}/../../lib/libc/sparc/gen/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.udiv')define(OP,\`div')define(S,\`false')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	@chmod 444 ${.TARGET}

rem.S: ${.CURDIR}/../../lib/libc/sparc/gen/divrem.m4
	echo 'building ${.TARGET} from ${.ALLSRC}'
	(echo "define(NAME,\`.rem')define(OP,\`rem')define(S,\`true')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	chmod 444 ${.TARGET}

urem.S: ${.CURDIR}/../../lib/libc/sparc/gen/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.urem')define(OP,\`rem')define(S,\`false')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	@chmod 444 ${.TARGET}

