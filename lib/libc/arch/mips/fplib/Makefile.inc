.PATH: ${.CURDIR}/arch/mips/fplib

SRCS+=fplib_libc.c fplib_glue.c softfloat.c

softfloat.o: softfloat.c
	${COMPILE.c} -freg-struct-return -c ${.IMPSRC} -o ${.TARGET}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

softfloat.po: softfloat.c
	${COMPILE.c} -freg-struct-return -p ${.IMPSRC} -o ${.TARGET}
	@${LD} -X -r ${.TARGET}
	@mv a.out ${.TARGET}

softfloat.so: softfloat.c
	${COMPILE.c} -freg-struct-return -c ${.IMPSRC} -o ${.TARGET}
