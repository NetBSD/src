#	$NetBSD: Makefile,v 1.1.1.1 1996/06/20 03:11:06 gwr Exp $

LIB=	kern

MACHINE_ARCH=m68k
MACHINE=sun3

# source tree is located via $S relative to the compilation directory
S=	../../../..
SRCDIR=	$S/lib/libkern

# CPPFLAGS=	-I${SRCDIR}/arch/${MACHINE_ARCH} \
# 		${KERNCPPFLAGS:S@-I.@-I${KERNREL}.@g}

INCLUDES=	-nostdinc -I. -I$S/arch -I$S
XDEFS= 		-undef -D__NetBSD__ -Dm68k -Dmc68000
DEFINES=	-D_KERNEL -Dmc68020 -Dsun3
CPPFLAGS=	${INCLUDES} ${XDEFS} ${DEFINES}
CFLAGS= 	${DEBUG} -O2 -msoft-float -Werror
APPFLAGS=	-P -traditional ${CPPFLAGS} -D_LOCORE

.if exists (${SRCDIR}/arch/${MACHINE_ARCH}/Makefile.inc)
.PATH:	${SRCDIR}/arch/${MACHINE_ARCH}
.include "${SRCDIR}/arch/${MACHINE_ARCH}/Makefile.inc"
.endif

.PATH: ${SRCDIR}

.if (${MACHINE_ARCH} != "alpha")
# Quad support
SRCS+=	adddi3.c anddi3.c ashldi3.c ashrdi3.c cmpdi2.c divdi3.c iordi3.c \
	lshldi3.c lshrdi3.c moddi3.c muldi3.c negdi2.c notdi2.c qdivrem.c \
	subdi3.c  ucmpdi2.c udivdi3.c umoddi3.c xordi3.c
.endif

# Other stuff
SRCS+=	

# Files to clean up
CLEANFILES+= lib${LIB}.o lib${LIB}.po

all: .links libkern.o libkern.po

install:

clean:
	rm -f libkern.o ${OBJS}
	rm -f libkern.po ${POBJS}
	rm .links

.links :
	-rm -f machine
	ln -s ../../include machine
	-rm -f ${MACHINE_ARCH}
	ln -s $S/arch/${MACHINE_ARCH}/include ${MACHINE_ARCH}
	touch .links

OBJS+=	${SRCS:N*.h:R:S/$/.o/g}
lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`

POBJS+=	${OBJS:.o=.po} mcount.po
lib${LIB}.po:: ${POBJS}
	@echo building profiled ${LIB} library
	@rm -f lib${LIB}.po
	@${LD} -r -o lib${LIB}.po `lorder ${POBJS} | tsort`

# mcount cannot be compiled with profiling
mcount.po: mcount.o
	cp mcount.o mcount.po

################################################################
# Stuff from bsd.lib.mk
################################################################

NOPIC=

# prefer .S to a .c, add .po, remove stuff not used in the BSD libraries.
# .so used for PIC object files.  .ln used for lint output files.
.SUFFIXES:
.SUFFIXES: .out .o .po .so .S .s .c .cc .C .f .y .l .ln .m4

.c.o:
	@echo "${COMPILE.c} ${.IMPSRC}"
	@${COMPILE.c} ${.IMPSRC}  -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.c.po:
	@echo "${COMPILE.c} -p ${.IMPSRC} -o ${.TARGET}"
	@${COMPILE.c} -p ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.S.o .s.o:
	@echo "${CPP} ${APPFLAGS} ${.IMPSRC} | \
		${AS} -o ${.TARGET}"
	@${CPP} ${APPFLAGS} ${.IMPSRC} | \
	    ${AS} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.S.po .s.po:
	@echo "${CPP} -DPROF ${APPFLAGS} ${.IMPSRC} |\
	    ${AS} -o ${.TARGET}"
	@${CPP} -DPROF ${APPFLAGS} ${.IMPSRC} | \
	    ${AS} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.include <bsd.obj.mk>
.include <bsd.dep.mk>
.include <bsd.sys.mk>
