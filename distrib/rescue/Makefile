#	$NetBSD: Makefile,v 1.1 2002/05/02 18:20:58 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

WARNS=		1
DBG=		-Os

CRUNCHBIN=	rescue
CRUNCHENV=	# empty - don't want SMALLPROG defined
LISTS=		${.CURDIR}/list
MTREECONF=	${.CURDIR}/mtree.conf
IMAGEDEPENDS=	${CRUNCHBIN}

.if (${MACHINE} == "pc532")
LISTS+=		${.CURDIR}/list.bim
.endif

.if (${MACHINE} == "atari")	|| (${MACHINE} == "mvme68k")	|| \
    (${MACHINE} == "sparc")	|| (${MACHINE} == "sparc64")	|| \
    (${MACHINE} == "sun2")	|| (${MACHINE} == "sun3")	|| \
    (${MACHINE} == "sun3x")
LISTS+=		${.CURDIR}/list.edlabel
.endif

.if (${MACHINE} == "arc")		|| (${MACHINE} == "bebox")	|| \
    (${MACHINE} == "cobalt")		|| (${MACHINE} == "hpcmips")	|| \
    (${MACHINE} == "i386")		|| (${MACHINE} == "mvmeppc")	|| \
    (${MACHINE} == "netwinder")		|| (${MACHINE} == "ofppc")	|| \
    (${MACHINE} == "playstation2")	|| (${MACHINE} == "prep")
LISTS+=		${.CURDIR}/list.fdisk
.endif

.if (${OBJECT_FMT} == "ELF")					# {

PARSELISTENV=	LDD_CMD=ldd_elf

.if (${MACHINE_ARCH} == "arm")	|| (${MACHINE_ARCH} == "i386")	|| \
    (${MACHINE_ARCH} == "m68k")	|| (${MACHINE_ARCH} == "sparc")	|| \
    (${MACHINE_ARCH} == "vax")
LISTS+=		${.CURDIR}/list.ldconfig
.endif

.else	# OBJECT_FMT != "ELF"					# } {

PARSELISTENV=	LDD_CMD=ldd_aout
LISTS+=		${.CURDIR}/list.ldconfig

.endif	# OBJECT_FMT != "ELF"					# }


.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.image"

realall: ${WORKBUILT} ${WORKSPEC}

install:
#	XXX: implement

.include <bsd.prog.mk>
