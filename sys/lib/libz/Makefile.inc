#	$NetBSD: Makefile.inc,v 1.1 1997/01/16 00:19:05 cgd Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

ZDIR=		$S/lib/libz

ZDST?=		${.OBJDIR}/lib/z
Z_AS?=		obj

.if (${Z_AS} == "library")
ZLIB=		${ZDST}/libz.a
.else
ZLIB=		${ZDST}/libz.o
.endif

ZMAKE= \
	cd ${ZDIR} && MAKEOBJDIR=${ZDST} ${MAKE} \
	    CC=${CC:Q} CFLAGS=${CFLAGS:Q} \
	    AS=${AS:Q} AFLAGS=${AFLAGS:Q} \
	    LD=${LD:Q} STRIP=${STRIP:Q} \
	    MACHINE=${MACHINE} MACHINE_ARCH=${MACHINE_ARCH:Q} \
	    ZCPPFLAGS=${CPPFLAGS:S@^-I.@-I../../.@g:Q}

${ZLIB}:		.NOTMAIN __always_make_zlib
	@echo making sure the z library is up to date...
.if (${Z_AS} == "library")
	@${ZMAKE} libz.a
.else
	@${ZMAKE} libz.o
.endif

clean:: 		.NOTMAIN __always_make_zlib
	@echo cleaning the z library objects
	@${ZMAKE} clean

depend:: 		.NOTMAIN __always_make_zlib
	@echo depending the z library objects
	@${ZMAKE} depend

__always_make_zlib:	.NOTMAIN
	@mkdir -p ${ZDST}
