#	$NetBSD: Makefile.inc,v 1.33 2009/08/12 21:20:41 dsl Exp $

SRCS+=	ffs.S
SRCS+=	memset.S
SRCS+=	strlen.S
SRCS+=	htonl.S htons.S ntohl.S ntohs.S
SRCS+=	random.S

SRCS+=  __main.c __assert.c
SRCS+=	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c
SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	memchr.c memcmp.c
SRCS+=	strcat.c strcmp.c strcpy.c strcasecmp.c
SRCS+=	strncasecmp.c strncmp.c strncpy.c strtoul.c
SRCS+=	scanc.c skpc.c

SRCS+=	mul.S umul.S saveregs.S

# `source' files built from m4 source
# the name `div.o' is taken for the ANSI C `div' function, hence sdiv here
SRCS+=	rem.S sdiv.S udiv.S urem.S
CLEANFILES+=rem.S sdiv.S udiv.S urem.S

.SUFFIXES: .m4

.PATH.m4: ${KERNDIR}/arch/${MACHINE_ARCH}

sdiv.S: divrem.m4
	echo 'building ${.TARGET} from ${.ALLSRC}'
	(echo "define(NAME,\`.div')define(OP,\`div')define(S,\`true')"; \
	 cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}

udiv.S: divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.udiv')define(OP,\`div')define(S,\`false')"; \
	 cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}

rem.S:	divrem.m4
	echo 'building ${.TARGET} from ${.ALLSRC}'
	(echo "define(NAME,\`.rem')define(OP,\`rem')define(S,\`true')"; \
	 cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}

urem.S: divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.urem')define(OP,\`rem')define(S,\`false')"; \
	 cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}
