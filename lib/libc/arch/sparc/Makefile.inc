#	$NetBSD: Makefile.inc,v 1.11 2005/10/15 22:11:22 uwe Exp $

KMINCLUDES= arch/sparc/SYS.h
KMSRCS=	bzero.S ffs.S strlen.S htonl.S htons.S \
	ntohl.S ntohs.S divrem.m4 \
	mul.S saveregs.S umul.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

# `source' files built from m4 source
# the name `div.o' is taken for the ANSI C `div' function, hence sdiv here
SRCS+=	rem.S sdiv.S udiv.S urem.S
CLEANFILES+=rem.S sdiv.S udiv.S urem.S

sdiv.S: ${.CURDIR}/arch/sparc/gen/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.div')define(OP,\`div')define(S,\`true')"; \
	 cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}
	@chmod 444 ${.TARGET}

udiv.S: ${.CURDIR}/arch/sparc/gen/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.udiv')define(OP,\`div')define(S,\`false')"; \
	 cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}
	@chmod 444 ${.TARGET}

rem.S: ${.CURDIR}/arch/sparc/gen/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.rem')define(OP,\`rem')define(S,\`true')"; \
	 cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}
	@chmod 444 ${.TARGET}

urem.S: ${.CURDIR}/arch/sparc/gen/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`.urem')define(OP,\`rem')define(S,\`false')"; \
	 cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}
	@chmod 444 ${.TARGET}
