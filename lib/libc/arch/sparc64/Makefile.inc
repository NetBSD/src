#	$NetBSD: Makefile.inc,v 1.5 2002/05/22 10:20:20 bjh21 Exp $

KMINCLUDES= arch/sparc64/SYS.h
KMSRCS=	bzero.S ffs.S strlen.S htonl.S htons.S \
	ntohl.S ntohs.S divrem.m4 \
	_setjmp.S mul.S saveregs.S setjmp.S umul.S

# Some assembler files use v9a extensions.
AFLAGS+= -Wa,-Av9a

# `source' files built from m4 source
# the name `div.o' is taken for the ANSI C `div' function, hence sdiv here
SRCS+=	rem.S sdiv.S udiv.S urem.S
CLEANFILES+=rem.S sdiv.S udiv.S urem.S

sdiv.S: ${.CURDIR}/arch/sparc64/gen/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.div')define(OP,\`div')define(S,\`true')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	@chmod 444 ${.TARGET}

udiv.S: ${.CURDIR}/arch/sparc64/gen/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.udiv')define(OP,\`div')define(S,\`false')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	@chmod 444 ${.TARGET}

rem.S: ${.CURDIR}/arch/sparc64/gen/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.rem')define(OP,\`rem')define(S,\`true')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	@chmod 444 ${.TARGET}

urem.S: ${.CURDIR}/arch/sparc64/gen/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.urem')define(OP,\`rem')define(S,\`false')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	@chmod 444 ${.TARGET}

.PATH: ${ARCHDIR}/softfloat
SRCS+= qp.c
CPPFLAGS+=	-DSOFTFLOATSPARC64_FOR_GCC
.if ${MKSOFTFLOAT} != "no"
.include <softfloat/Makefile.inc>
.else
#
# Can't directly use softfloat.c due to .PATH issues with the other source
# in softfloat. Just provide a wrapper which pulls it in via a #include
SRCS+= softfloat-wrapper.c
CPPFLAGS+=      -I${ARCHDIR}/softfloat -I${.CURDIR}/softfloat \
		-I${.CURDIR}/softfloat/bits64
.endif
