#	$NetBSD: Makefile,v 1.32 2001/05/06 13:54:04 wiz Exp $

.include <bsd.own.mk>
.if (${OBJECT_FMT} == "a.out")
PROG=	ld.so
SRCS=	mdprologue.S rtld.c malloc.c shlib.c md.c vfprintf.c
CLIB=	${.CURDIR}/../../lib/libc
CLIBOBJ!=cd ${CLIB}; \
	 printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
PICFLAG=-fpic -fno-function-cse
CPPFLAGS+=$(PICFLAG) -DRTLD -DLIBC_SCCS \
	  -I${CLIB}/include -I$(.CURDIR)/arch/$(MACHINE_ARCH) -I$(.CURDIR)
.if defined(DEBUG)
CPPFLAGS+=-DDEBUG
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
.elif (${OBJECT_FMT} == "ELF" && \
       (${MACHINE_ARCH} == "i386" || \
        ${MACHINE_ARCH} == "m68k" || \
        ${MACHINE_ARCH} == "sparc"))

FILES=		ld.so
FILESDIR=	/usr/libexec
FILESMODE=	${BINMODE}
CLEANFILES+=	ld.so

all dependall: ${FILES}

ld.so:	${.CURDIR}/ld.so.${MACHINE_ARCH}.uue
	uudecode ${.ALLSRC}

.include <bsd.files.mk>

.endif

MAN= ld.aout_so.1

.include <bsd.prog.mk>
