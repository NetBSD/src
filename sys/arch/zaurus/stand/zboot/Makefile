#	$NetBSD: Makefile,v 1.3.4.2 2009/05/13 17:18:54 jym Exp $

PROG=		zboot

S=		${.CURDIR}/../../../..

SRCS=		crt0.c
SRCS+=		boot.c bootinfo.c bootmenu.c conf.c devopen.c diskprobe.c
SRCS+=		loadfile_zboot.c
SRCS+=		getsecs.c termios.c unixcons.c unixdev.c unixsys.S

NOMAN=		# defined

CFLAGS+=	-Wall -Wno-main
CFLAGS+=	-Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
CFLAGS+=	-fno-stack-protector -fno-builtin -ffreestanding
CPPFLAGS+=	-nostdinc -I. -I${.CURDIR} -I${.OBJDIR} -I${S}
CPPFLAGS+=	-D_STANDALONE -DHEAP_VARIABLE
AFLAGS+=	-D_LOCORE
LDFLAGS+=	-nostdlib -Bstatic

CLEANFILES+=	vers.c vers.o

LIBCRT0=	crt0.o
LIBC=		# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing

NEWVERSWHAT?=   "Boot"
VERSIONFILE?=	${.CURDIR}/version

.if !make(obj) && !make(clean) && !make(cleandir) && !make(release)
.BEGIN: machine arm
.NOPATH: machine arm

machine::
	-rm -f $@
	ln -s ${S}/arch/${MACHINE}/include $@

arm::
	-rm -f $@
	ln -s ${S}/arch/arm/include $@
.endif
CLEANFILES+=	machine arm

### find out what to use for libkern
KERN_AS=		library
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

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${HOST_SH} ${S}/conf/newvers_stand.sh ${VERSIONFILE} ${MACHINE} ${NEWVERSWHAT}
	${CC} -c vers.c
	${LD} ${LDFLAGS} -o ${PROG} ${OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBKERN}

.include <bsd.prog.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG} \
	    ${RELEASEDIR}/${MACHINE}/installation
