#	$NetBSD: Makefile,v 1.58.10.1 2018/06/25 07:25:44 pgoyette Exp $

S=	${.CURDIR}/../../../..

PROG=		ofwboot
FILES=		${PROG}.elf ${PROG}.xcf
SRCS=		Locore.c boot.c ofdev.c hfs.c net.c netif_of.c
XCOFFXTRA=	Xcoffxtra.c
XCOFFXTRAOBJ=	Xcoffxtra.o
CFLAGS+=	-ffreestanding
CFLAGS+=	${${ACTIVE_CC} == "gcc":? -msoft-float :}
CFLAGS+=	${CFLAGS_UNWIND.${ACTIVE_CC}}
CFLAGS+=	-Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
#CPPFLAGS+=	-DDEBUG -DNETIF_DEBUG
CPPFLAGS+=	-D_STANDALONE -DSUPPORT_DHCP
CPPFLAGS+=	-DSUPPORT_USTARFS -DHAVE_CHANGEDISK_HOOK
DBG=		-Os

# PR port-macppc/44895
SRCS+=		alloc.c			# use ./alloc.c
#CPPFLAGS+=	-DHEAP_VARIABLE		# use libsa alloc.c

LIBCRT0=	# nothing
LIBCRTI=	# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing
LIBC=		# nothing

.PATH:		${S}/arch/powerpc/powerpc
SRCS+=		ofwmagic.S

.PATH:		${S}/lib/libsa
SRCS+=		byteorder.c

NOMAN=		# defined
STRIPFLAG=
BINMODE=	444

.include <bsd.own.mk>

.if !defined(FIXCOFF)
.if !defined(TOOL_MACPPCFIXCOFF)
TOOL_MACPPCFIXCOFFDIR!=	cd ${.CURDIR}/../fixcoff && ${PRINTOBJDIR}
TOOL_MACPPCFIXCOFF=	${TOOL_MACPPCFIXCOFFDIR}/fixcoff
.endif
.endif

NEWVERSWHAT=	"OpenFirmware Boot"

# For now...
#RELOC=		20000
RELOC=		E00000

ENTRY=		_start

CLEANFILES+=	${PROG}.elf ${PROG}.el1 ${PROG}.mrg ${PROG}.xcf
CLEANFILES+=	${XCOFFXTRAOBJ}

CPPFLAGS+=	-I. -I${.CURDIR} -I${.CURDIR}/../../.. -I${.CURDIR}/../../../..
CPPFLAGS+=	-DRELOC=0x${RELOC} -DRELOC_FLATFILE=0x${RELOC_FLATFILE}
#CPPFLAGS+=	-DXCOFF_GLUE		# for booting PCI Powermacs

LINKFLAGS=	-N -Ttext ${RELOC} -Bstatic
LINKFLAGS+=	${LINKFLAGS_UNWIND}

### find out what to use for libkern
KERN_AS=	library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

### find out what to use for libz
Z_AS=		library
.include "${S}/lib/libz/Makefile.inc"
LIBZ=		${ZLIB}

### find out what to use for libsa
SA_AS=		library
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib

.include "${S}/conf/newvers_stand.mk"

all realall: ${PROG} ${PROG}.xcf ${PROG}.elf

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN} 
	${_MKTARGET_LINK}
	${LD} -s -o ${PROG}.el1 ${LINKFLAGS} \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -O binary ${PROG}.el1 ${PROG}

.include <bsd.klinks.mk>
.include <bsd.prog.mk>

${PROG}.elf: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${_MKTARGET_LINK}
	${LD} -s -o ${PROG}.elf ${LINKFLAGS} \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}

${PROG}.xcf: ${OBJS} ${XCOFFXTRAOBJ} ${LIBSA} ${LIBZ} ${LIBKERN}
	${_MKTARGET_LINK}
	${LD} -o ${PROG}.mrg ${LINKFLAGS} \
	    -T ${.CURDIR}/../fixcoff/elf32_powerpc_merge.x \
	    -e _entry ${XCOFFXTRAOBJ} \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -O aixcoff-rs6000 -R .comment -R .note \
	    ${PROG}.mrg ${PROG}.xcf
	${TOOL_MACPPCFIXCOFF} ${PROG}.xcf
