#	$NetBSD: Makefile,v 1.13 1997/05/29 15:13:07 cjs Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/1/93

CFLAGS+=	-DLIBC_SCCS -fPIC -DPIC -DDYNAMIC -DELFSIZE=32
#CFLAGS+=	-I/usr/src/libexec/ld.elf_so

OBJS=		crt0.o gcrt0.o crtn.o # crtbegin.o crtend.o
CLEANFILES+=	core a.out

.include	<bsd.own.mk>	# for OBJDIR and BUILDDIR

all: ${OBJS}

crt0.o: crt0.c
	@echo ${CC} ${CFLAGS} -DCRT0 -c ${.ALLSRC} -o ${.TARGET}
	@${CC} ${CFLAGS} -DCRT0 -c ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o
.if defined(OBJDIR)
	@echo install -d ${BUILDDIR}${LIBDIR}
	@install -d ${BUILDDIR}${LIBDIR}
	@echo ${BUILDDIR}${LIBDIR}/${.TARGET} '->' `pwd`/${.TARGET};
	@rm -f ${BUILDDIR}${LIBDIR}/${.TARGET};
	@ln -s `pwd`/${.TARGET} ${BUILDDIR}${LIBDIR}/${.TARGET};
.endif

gcrt0.o: crt0.c
	@echo ${CC} ${CFLAGS} -DMCRT0 -c ${.ALLSRC} -o ${.TARGET}
	@${CC} ${CFLAGS} -DMCRT0 -c ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o
.if defined(OBJDIR)
	@echo install -d ${BUILDDIR}${LIBDIR}
	@install -d ${BUILDDIR}${LIBDIR}
	@echo ${BUILDDIR}${LIBDIR}/${.TARGET} '->' `pwd`/${.TARGET};
	@rm -f ${BUILDDIR}${LIBDIR}/${.TARGET};
	@ln -s `pwd`/${.TARGET} ${BUILDDIR}${LIBDIR}/${.TARGET};
.endif

crtn.o: crtend.c
	@echo ${CC} ${CFLAGS} -DMCRT0 -c ${.ALLSRC} -o ${.TARGET}
	@${CC} ${CFLAGS} -DMCRT0 -c ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o
.if defined(OBJDIR)
	@echo install -d ${BUILDDIR}${LIBDIR}
	@install -d ${BUILDDIR}${LIBDIR}
	@echo ${BUILDDIR}${LIBDIR}/${.TARGET} '->' `pwd`/${.TARGET};
	@rm -f ${BUILDDIR}${LIBDIR}/${.TARGET};
	@ln -s `pwd`/${.TARGET} ${BUILDDIR}${LIBDIR}/${.TARGET};
.endif

install:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib
	ln -f ${DESTDIR}/usr/lib/crt0.o ${DESTDIR}/usr/lib/scrt0.o

.include <bsd.prog.mk>
