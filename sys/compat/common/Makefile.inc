#	$NetBSD: Makefile.inc,v 1.8 1996/08/10 03:00:51 mycroft Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

COMPATDIR=	$S/compat/common

COMPATDST?=	${.OBJDIR}/lib/compat
COMPATLIB=	${COMPATDST}/libcompat.a
COMPATLIB_PROF=	${COMPATDST}/libcompat_p.a

COMPATMAKE= \
	cd ${COMPATDIR} && MAKEOBJDIR=${COMPATDST} ${MAKE} \
	    CC='${CC}' CFLAGS='${CFLAGS}' \
	    LD='${LD}' \
	    COMPATCPPFLAGS='${CPPFLAGS:S@-I.@-I${.OBJDIR}@}'

${COMPATLIB}:		.NOTMAIN __always_make_compatlib
	@echo making sure the compat library is up to date...
	@${COMPATMAKE} libcompat.a

${COMPATLIB_PROF}:	.NOTMAIN __always_make_compatlib
	@echo making sure the profiled compat library is up to date...
	@${COMPATMAKE} libcompat_p.a

clean:: 		.NOTMAIN __always_make_compatlib
	@echo cleaning the compat library objects
	@${COMPATMAKE} clean

depend:: 		.NOTMAIN __always_make_compatlib
	@echo depending the compat library objects
	@${COMPATMAKE} depend

__always_make_compatlib:	.NOTMAIN
	@mkdir -p ${COMPATDST}
