#	$NetBSD: Makefile.inc,v 1.16 1996/08/08 15:06:43 mycroft Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

KERNDIR=	$S/lib/libkern

KERNDST?=	${.OBJDIR}/lib/kern
KERNLIB=	${KERNDST}/libkern.o
KERNLIB_PROF=	${KERNDST}/libkern.po

KERNMAKE= \
	cd ${KERNDIR} && MAKEOBJDIR=${KERNDST} ${MAKE} \
	    CC='${CC}' CFLAGS='${CFLAGS}' \
	    AS='${AS}' AFLAGS='${AFLAGS}' \
	    LD='${LD}' \
	    MACHINE='${MACHINE}' MACHINE_ARCH='${MACHINE_ARCH}' \
	    KERNCPPFLAGS='${CPPFLAGS}' \
	    KERNDIR='${KERNDIR}'

${KERNLIB}:		.NOTMAIN __always_make_kernlib
	@echo making sure the kern library is up to date...
	@${KERNMAKE} libkern.o

${KERNLIB_PROF}:	.NOTMAIN __always_make_kernlib
	@echo making sure the profiled kern library is up to date...
	@${KERNMAKE} libkern.po

clean:: 		.NOTMAIN __always_make_kernlib
	@echo cleaning the kern library objects
	@${KERNMAKE} clean

depend:: 		.NOTMAIN __always_make_kernlib
	@echo depending the kern library objects
	@${KERNMAKE} depend

__always_make_kernlib:	.NOTMAIN
	@mkdir -p ${KERNDST}
