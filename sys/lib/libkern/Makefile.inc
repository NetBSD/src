#	$NetBSD: Makefile.inc,v 1.23 1996/12/26 21:32:37 christos Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

KERNDIR=	$S/lib/libkern

KERNDST?=	${.OBJDIR}/lib/kern
KERN_AS?=	obj

.if (${KERN_AS} == "library")
KERNLIB=	${KERNDST}/libkern.a
KERNLIB_PROF=	${KERNDST}/libkern_p.a
.else
KERNLIB=	${KERNDST}/libkern.o
KERNLIB_PROF=	${KERNDST}/libkern.po
.endif

KERNMAKE= \
	cd ${KERNDIR} && MAKEOBJDIR=${KERNDST} ${MAKE} \
	    CC=${CC:Q} CFLAGS=${CFLAGS:Q} \
	    AS=${AS:Q} AFLAGS=${AFLAGS:Q} \
	    LD=${LD:Q} STRIP=${STRIP:Q} \
	    MACHINE=${MACHINE} MACHINE_ARCH=${MACHINE_ARCH:Q} \
	    KERNCPPFLAGS=${CPPFLAGS:S@^-I.@-I../../.@g:Q}

${KERNLIB}:		.NOTMAIN __always_make_kernlib
	@echo making sure the kern library is up to date...
.if (${KERN_AS} == "library")
	@${KERNMAKE} libkern.a
.else
	@${KERNMAKE} libkern.o
.endif

${KERNLIB_PROF}:	.NOTMAIN __always_make_kernlib
	@echo making sure the profiled kern library is up to date...
.if (${KERN_AS} == "library")
	@${KERNMAKE} libkern_p.a
.else
	@${KERNMAKE} libkern.po
.endif

clean:: 		.NOTMAIN __always_make_kernlib
	@echo cleaning the kern library objects
	@${KERNMAKE} clean

depend:: 		.NOTMAIN __always_make_kernlib
	@echo depending the kern library objects
	@${KERNMAKE} depend

__always_make_kernlib:	.NOTMAIN
	@mkdir -p ${KERNDST}
