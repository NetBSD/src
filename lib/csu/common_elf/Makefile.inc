#	$NetBSD: Makefile.inc,v 1.21 2003/03/18 19:11:57 christos Exp $

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

.PATH:		${.CURDIR}/../common_elf

SRCS+=		crt0.c crti.c crtn.c
OBJS+=		crt0.o gcrt0.o crti.o crtn.o
.if ${MKPIC} != "no"
COPTS+=		-fPIC
.endif

# If using an external toolchain, we expect crtbegin/crtend to be
# supplied by that toolchain's run-time support.
.if !defined(EXTERNAL_TOOLCHAIN)
SRCS+=		crtbegin.c crtend.c
OBJS+=		crtbegin.o crtend.o
.if ${MKPIC} != "no"
OBJS+=		crtbeginS.o crtendS.o
.endif
.endif

realall: ${OBJS}

crt0.o: crt0.c
	@echo "${COMPILE.c} -DCRT0 ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} -DCRT0 ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

gcrt0.o: crt0.c
	@echo "${COMPILE.c} -DMCRT0 ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} -DMCRT0 ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crti.o: crti.c
	@echo "${COMPILE.c} ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -X -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtn.o: crtn.c
	@echo "${COMPILE.c} ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -X -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

.if !defined(EXTERNAL_TOOLCHAIN)
crtbegin.o: crtbegin.c
	@echo "${COMPILE.c} ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -X -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtbeginS.o: crtbegin.c
	@echo "${COMPILE.c} -DSHARED ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} -DSHARED ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -X -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtend.o: crtend.c
	@echo "${COMPILE.c} ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -X -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtendS.o: crtend.c
	@echo "${COMPILE.c} -DSHARED ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} -DSHARED ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -X -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o
.endif	# ! EXTERNAL_TOOLCHAIN

FILES=${OBJS}
FILESDIR=${LIBDIR}

.include <bsd.prog.mk>
