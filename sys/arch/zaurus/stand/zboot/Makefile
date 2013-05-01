#	$NetBSD: Makefile,v 1.9 2013/05/01 22:16:16 matt Exp $

PROG=		zboot

S=		${.CURDIR}/../../../..

SRCS=		crt0.c
SRCS+=		boot.c bootinfo.c bootmenu.c conf.c devopen.c diskprobe.c
SRCS+=		loadfile_zboot.c
SRCS+=		getsecs.c termios.c unixcons.c unixdev.c unixsys.S
SRCS+=		pathfs.c

NOMAN=		# defined

.include <bsd.own.mk>

CFLAGS+=	-Wall -Wno-main
CFLAGS+=	-Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
CFLAGS+=	-fno-stack-protector -fno-builtin -ffreestanding
CPPFLAGS+=	-nostdinc -I. -I${.CURDIR} -I${.OBJDIR} -I${S}
CPPFLAGS+=	-D_STANDALONE -DHEAP_VARIABLE
CPPFLAGS+=	-DLIBSA_ENABLE_LS_OP
CPPFLAGS+=	-mabi=apcs-gnu -mfloat-abi=soft -mcpu=xscale
CPUFLAGS=
AFLAGS+=	-D_LOCORE
LDFLAGS+=	-nostdlib -Bstatic

CLEANFILES+=	vers.c vers.o

LIBCRT0=	crt0.o
LIBC=		# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing

NEWVERSWHAT?=   "Boot"
VERSIONFILE?=	${.CURDIR}/version

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
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes SA_ENABLE_LS_OP=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN} ${VERSIONFILE}
	${_MKTARGET_LINK}
	${HOST_SH} ${S}/conf/newvers_stand.sh ${${MKREPRO} == "yes" :?:-D} \
	    ${VERSIONFILE} ${MACHINE} ${NEWVERSWHAT}
	${CC} -c vers.c
	${LD} ${LDFLAGS} -o ${PROG} ${OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBKERN}

.include <bsd.klinks.mk>
.include <bsd.prog.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG} \
	    ${RELEASEDIR}/${MACHINE}/installation
