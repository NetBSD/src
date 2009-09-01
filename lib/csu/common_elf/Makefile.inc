#	$NetBSD: Makefile.inc,v 1.30.10.1 2009/09/01 23:09:14 matt Exp $

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

.if defined(HAVE_GCC) && ${HAVE_GCC} > 3
CFLAGS+=	-fno-unit-at-a-time
.endif

.PATH:		${.CURDIR}/../common_elf

SRCS+=		crt0.c crti.c crtn.c
OBJS+=		crt0.o gcrt0.o crti.o crtn.o
.if ${MKPIC} != "no"
COPTS+=		-fPIC
.endif

realall: ${OBJS}

crt0.o: crt0.c
	${_MKTARGET_COMPILE}
	${COMPILE.c} -DCRT0 ${.IMPSRC} -o ${.TARGET}
	${OBJCOPY} -x ${.TARGET}

# dependant on crt0.o to pick up header dependencies
gcrt0.o: ${.CURDIR}/crt0.c crt0.o
	${_MKTARGET_COMPILE}
	${COMPILE.c} -DMCRT0 ${.CURDIR}/crt0.c -o ${.TARGET}
	${OBJCOPY} -x ${.TARGET}

crti.o: crti.c
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${.IMPSRC} -o ${.TARGET}
	${OBJCOPY} -X ${.TARGET}

crtn.o: crtn.c
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${.IMPSRC} -o ${.TARGET}
	${OBJCOPY} -X ${.TARGET}

FILES=${OBJS}
FILESDIR=${LIBDIR}
CLEANFILES=${OBJS}

.include <bsd.prog.mk>
