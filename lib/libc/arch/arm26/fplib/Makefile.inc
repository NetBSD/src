#	$NetBSD: Makefile.inc,v 1.1 2000/05/09 21:55:46 bjh21 Exp $

.PATH: ${.CURDIR}/arch/arm26/fplib

SRCS+=fplib_glue.S fplib_libc.c softfloat.c

softfloat.o: softfloat.c
	@echo ${COMPILE.c:Q} -freg-struct-return ${.IMPSRC}
	@${COMPILE.c} -freg-struct-return ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

softfloat.po: softfloat.c
	@echo ${COMPILE.c:Q} -pg -freg-struct-return ${.IMPSRC} -o ${.TARGET}
	@${COMPILE.c} -pg -freg-struct-return ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

softfloat.so: softfloat.c
	@echo ${COMPILE.c:Q} ${CPICFLAGS} -freg-struct-return ${.IMPSRC} -o ${.TARGET}
	@${COMPILE.c} ${CPICFLAGS} -freg-struct-return ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o
