#	$Id: Makefile.inc,v 1.3 1993/10/15 08:41:25 cgd Exp $

SRCS+=  __main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bcmp.c bzero.s ffs.s strcat.c strcmp.c strcpy.c strlen.s strncmp.c \
	strncpy.c htonl.s htons.s ntohl.s ntohs.s

SRCS+=	umul.s mul.s rem.s sdiv.s udiv.s umul.s urem.s
SRCS+=	_setjmp.s fabs.s fixunsdfsi.s modf.s mul.s saveregs.s setjmp.s umul.s
SRCS+=	frexp.c isinf.c isnan.c ldexp.c

# `source' files built from m4 source
# the name `div.o' is taken for the ANSI C `div' function, hence sdiv here
SRCS+=	rem.s sdiv.s udiv.s urem.s
CLEANFILES+=rem.s sdiv.s udiv.s urem.s

## Temporary hack for Theo's cross compile environment
CFLAGS+=-I/tmp/usr/include

sdiv.s: ${.CURDIR}/../../lib/libc/sparc/gen/divrem.m4
	echo 'building ${.TARGET} from ${.ALLSRC}'
	(echo "define(NAME,\`.div')define(OP,\`div')define(S,\`true')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	chmod 444 ${.TARGET}

udiv.s: ${.CURDIR}/../../lib/libc/sparc/gen/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.udiv')define(OP,\`div')define(S,\`false')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	@chmod 444 ${.TARGET}

rem.s: ${.CURDIR}/../../lib/libc/sparc/gen/divrem.m4
	echo 'building ${.TARGET} from ${.ALLSRC}'
	(echo "define(NAME,\`.rem')define(OP,\`rem')define(S,\`true')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	chmod 444 ${.TARGET}

urem.s: ${.CURDIR}/../../lib/libc/sparc/gen/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.urem')define(OP,\`rem')define(S,\`false')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	@chmod 444 ${.TARGET}

