#	$NetBSD: Makefile,v 1.41 2003/07/10 10:34:25 lukem Exp $

.include <bsd.own.mk>			# for OBJECT_FMT definition
.include <bsd.shlib.mk>			# for SHLINKINSTALLDIR definition
.if (${OBJECT_FMT} == "a.out")
PROG=	ld.so
SRCS=	mdprologue.S rtld.c malloc.c shlib.c md.c vfprintf.c
BINDIR=	${SHLINKINSTALLDIR}

CLIB=	${NETBSDSRCDIR}/lib/libc
CLIBOBJ!=cd ${CLIB} && ${PRINTOBJDIR}
PICFLAG=-fpic -fno-function-cse
CPPFLAGS+=$(PICFLAG) -DRTLD -DLIBC_SCCS \
	  -I${CLIB}/include -I$(.CURDIR)/arch/$(MACHINE_ARCH) -I$(.CURDIR)
.if defined(DEBUG)
CPPFLAGS+=-DDEBUG
.endif
.if ${SHLIBINSTALLDIR} != ${LIBDIR}
CPPFLAGS+=-DSTANDARD_SEARCH_DIRS=\"${SHLIBINSTALLDIR}\",\"${LIBDIR}\"
.endif
ASFLAGS+=-k
LDFLAGS+=-Bshareable -Bsymbolic -assert nosymbolic -L${CLIBOBJ}
.if defined(DESTDIR)
LDFLAGS+= -nostdlib -L${DESTDIR}/usr/lib
.endif
LDADD+=	-lc_pic
DPADD+=	${LIBC_PIC}

.PATH: ${CLIB}/stdio $(.CURDIR)/arch/$(MACHINE_ARCH)

$(PROG):
	$(LD) -o $(PROG) $(LDFLAGS) $(OBJS) $(LDADD)

.S.o:
	${CPP} ${.IMPSRC} | ${AS} ${ASFLAGS} -o ${.TARGET} -

.if ${SHLINKINSTALLDIR} != "/usr/libexec"
SYMLINKS+=	${SHLINKINSTALLDIR}/${PROG} /usr/libexec/${PROG}
.endif

.elif (${OBJECT_FMT} == "ELF" && \
       (${MACHINE_ARCH} == "arm" || \
        ${MACHINE_ARCH} == "i386" || \
        ${MACHINE_ARCH} == "m68k" || \
        ${MACHINE_ARCH} == "sparc" || \
        ${MACHINE_ARCH} == "vax"))

FILES=		ld.so
FILESDIR=	/usr/libexec
FILESMODE=	${BINMODE}
CLEANFILES+=	ld.so

all dependall: ${FILES}

ld.so:	${.CURDIR}/ld.so.${MACHINE_ARCH}.uue
	${TOOL_UUDECODE} ${.ALLSRC}

.endif

MAN= ld.aout_so.1

.include <bsd.prog.mk>
