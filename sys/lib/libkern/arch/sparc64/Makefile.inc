#	$NetBSD: Makefile.inc,v 1.4.2.1 2002/06/20 03:47:35 nathanw Exp $

SRCS+=  __main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c \
	bcmp.c bzero.S ffs.S \
	memchr.c memcmp.c memset.c \
	strcat.c strcmp.c strcpy.c strtoul.c strlen.S strcasecmp.c \
	strncasecmp.c strncmp.c strncpy.c \
	scanc.c skpc.c \
	htonl.S htons.S ntohl.S ntohs.S \
	random.S

SRCS+=	umul.S mul.S rem.S sdiv.S udiv.S umul.S urem.S
SRCS+=	mul.S saveregs.S umul.S

# `source' files built from m4 source
# the name `div.o' is taken for the ANSI C `div' function, hence sdiv here
SRCS+=	rem.S sdiv.S udiv.S urem.S
CLEANFILES+=rem.S sdiv.S udiv.S urem.S

sdiv.S: $M/divrem.m4
	echo 'building ${.TARGET} from ${.ALLSRC}'
	(echo "define(NAME,\`.div')define(OP,\`div')define(S,\`true')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}

udiv.S: $M/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.udiv')define(OP,\`div')define(S,\`false')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}

rem.S:	$M/divrem.m4
	echo 'building ${.TARGET} from ${.ALLSRC}'
	(echo "define(NAME,\`.rem')define(OP,\`rem')define(S,\`true')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}

urem.S: $M/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.urem')define(OP,\`rem')define(S,\`false')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
