.PATH: ${.CURDIR}/arch/arm32/fplib

SRCS+=fplib_glue.S fplib_libc.c softfloat.c

softfloat.o: softfloat.c
	${CC} -freg-struct-return ${CFLAGS} ${CPPFLAGS} -c ${.IMPSRC} \
	    -o ${.TARGET}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

softfloat.po: softfloat.c
	${CC} -freg-struct-return ${CFLAGS} ${CPPFLAGS} -c -p ${.IMPSRC} \
	    -o ${.TARGET}
	@${LD} -X -r ${.TARGET}
	@mv a.out ${.TARGET}

softfloat.so: softfloat.c
	${CC} ${CPICFLAGS} -freg-struct-return ${CFLAGS} ${CPPFLAGS} \
	    -c ${.IMPSRC} -o ${.TARGET}
