#	$NetBSD: Makefile,v 1.8 1997/05/26 03:56:31 cjs Exp $

CFLAGS+=	-DLIBC_SCCS
.if (${MACHINE_ARCH} != "powerpc")
CFLAGS+=	-fpic
.endif
OBJS=		c++rt0.o

.include <bsd.own.mk>	# for BUILDDIR

all: ${OBJS}

c++rt0.o: c++rt0.c
	${COMPILE.c} ${.ALLSRC}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}
.if defined(OBJDIR)
	@echo install -d ${BUILDDIR}${LIBDIR}
	@install -d ${BUILDDIR}${LIBDIR}
	@echo ${BUILDDIR}${LIBDIR}/${.TARGET} '->' `pwd`/${.TARGET};
	@rm -f ${BUILDDIR}${LIBDIR}/${.TARGET};
	@ln -s `pwd`/${.TARGET} ${BUILDDIR}${LIBDIR}/${.TARGET};
.endif

FILES=${OBJS}
FILESDIR=/usr/lib

.include <bsd.prog.mk>
