#	$NetBSD: Makefile.inc,v 1.27 2009/08/12 21:20:40 dsl Exp $

SRCS+=	_mcount.S
SRCS+=	byte_swap_2.S byte_swap_4.S
SRCS+=	ffs.S
SRCS+=	memcpy.S memmove.S

SRCS+=	__main.c __assert.c
SRCS+=	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c
SRCS+=	bswap64.c
SRCS+=	memchr.c memcmp.c memset.c
SRCS+=	softfloat.c
SRCS+=	strcat.c strcmp.c strcpy.c strlen.c strcasecmp.c
SRCS+=	strncasecmp.c strncmp.c strncpy.c strtoul.c
SRCS+=	scanc.c skpc.c
SRCS+=	random.c

# `source' files built from m4 source
SRCS+=	__divqu.S __divq.S __divlu.S __divl.S
SRCS+=	__remqu.S __remq.S __remlu.S __reml.S
CLEANFILES+=	__divqu.S __divq.S __divlu.S __divl.S
CLEANFILES+=	__remqu.S __remq.S __remlu.S __reml.S

__divqu.S: ${M}/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`__divqu')define(OP,\`div')define(S,\`false')"; \
	 echo "define(WORDSIZE,64)"; cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}

__divq.S: ${M}/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`__divq')define(OP,\`div')define(S,\`true')"; \
	 echo "define(WORDSIZE,64)"; cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}

__divlu.S: ${M}/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`__divlu')define(OP,\`div')define(S,\`false')"; \
	 echo "define(WORDSIZE,32)"; cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}

__divl.S: ${M}/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`__divl')define(OP,\`div')define(S,\`true')"; \
	 echo "define(WORDSIZE,32)"; cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}

__remqu.S: ${M}/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`__remqu')define(OP,\`rem')define(S,\`false')"; \
	 echo "define(WORDSIZE,64)"; cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}

__remq.S: ${M}/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`__remq')define(OP,\`rem')define(S,\`true')"; \
	 echo "define(WORDSIZE,64)"; cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}

__remlu.S: ${M}/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`__remlu')define(OP,\`rem')define(S,\`false')"; \
	 echo "define(WORDSIZE,32)"; cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}

__reml.S: ${M}/divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@(echo "define(NAME,\`__reml')define(OP,\`rem')define(S,\`true')"; \
	 echo "define(WORDSIZE,32)"; cat ${.ALLSRC}) | ${TOOL_M4} > ${.TARGET}
