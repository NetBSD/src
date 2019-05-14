.include "../Makefile.inc"

.if ${MACHINE_ARCH} == "powerpc"
CFLAGS	+= -msecure-plt
.endif
