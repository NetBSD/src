#	$NetBSD: Makefile.inc,v 1.26 2006/05/13 01:40:34 mrg Exp $

.include <bsd.own.mk>

.if !defined(ELFSIZE)
ELFSIZE=32
.endif

.include "../../Makefile.inc"

CPPFLAGS+=	-DLIBC_SCCS -DPIC -DDYNAMIC -DELFSIZE=${ELFSIZE}
CPPFLAGS+=	-I${NETBSDSRCDIR}/libexec/ld.elf_so
CPPFLAGS+=	-I${.CURDIR}/../common_elf
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/dlfcn
CPPFLAGS+=	-DDWARF2_EH
CPPFLAGS+=	-DJCR
CPPFLAGS+=	-DDSO_HANDLE

.if ${HAVE_GCC} > 3
CFLAGS+=	-fno-unit-at-a-time
.endif

.PATH:		${.CURDIR}/../common_elf

SRCS+=		crt0.c crti.c crtn.c
OBJS+=		crt0.o gcrt0.o crti.o crtn.o
.if ${MKPIC} != "no"
COPTS+=		-fPIC
.endif

# We supply crtbegin / crtend *ONLY* if using the old in-tree GCC 2.95.3!
# In all other cases (including use of an external toolchain), these will
# be provided by that toolchain's run-time support.
.if ${USE_TOOLS_TOOLCHAIN} == "yes" && !defined(EXTERNAL_TOOLCHAIN)
SRCS+=		crtbegin.c crtend.c
OBJS+=		crtbegin.o crtend.o
.if ${MKPIC} != "no"
SRCS+=		crtbeginS.c crtendS.c
OBJS+=		crtbeginS.o crtendS.o
.endif
.endif

realall: ${OBJS}

crt0.o: crt0.c
	@echo "${COMPILE.c} -DCRT0 ${.IMPSRC} -o ${.TARGET}"
	@${COMPILE.c} -DCRT0 ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

# dependant on crt0.o to pick up header dependencies
gcrt0.o: ${.CURDIR}/crt0.c crt0.o
	@echo "${COMPILE.c} -DMCRT0 ${.CURDIR}/crt0.c -o ${.TARGET}"
	@${COMPILE.c} -DMCRT0 ${.CURDIR}/crt0.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crti.o: crti.c
	@echo "${COMPILE.c} ${.IMPSRC} -o ${.TARGET}"
	@${COMPILE.c} ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -X -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtn.o: crtn.c
	@echo "${COMPILE.c} ${.IMPSRC} -o ${.TARGET}"
	@${COMPILE.c} ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -X -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

.if ${USE_TOOLS_TOOLCHAIN} == "yes" && !defined(EXTERNAL_TOOLCHAIN)
crtbegin.o: crtbegin.c
	@echo "${COMPILE.c} ${.IMPSRC} -o ${.TARGET}"
	@${COMPILE.c} ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -X -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtbeginS.o: crtbeginS.c
	@echo "${COMPILE.c} -DSHARED ${.IMPSRC} -o ${.TARGET}"
	@${COMPILE.c} -DSHARED ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -X -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtend.o: crtend.c
	@echo "${COMPILE.c} ${.IMPSRC} -o ${.TARGET}"
	@${COMPILE.c} ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -X -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtendS.o: crtendS.c
	@echo "${COMPILE.c} -DSHARED ${.IMPSRC} -o ${.TARGET}"
	@${COMPILE.c} -DSHARED ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -X -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o
.endif	# USE_TOOLS_TOOLCHAIN == "yes" && ! EXTERNAL_TOOLCHAIN

FILES=${OBJS}
FILESDIR=${LIBDIR}

.include <bsd.prog.mk>
