#	$NetBSD: Makefile,v 1.1 2013/08/20 17:35:41 matt Exp $

WITHOUT_MAN=true  # defined

.include <bsd.obj.mk>		# Pull in OBJDIR name rules.
.include <bsd.own.mk>

LIB=	kern
DBG=	-Os

MKDEBUGLIB:=    no
MKLINT:=        no
MKPICINSTALL:=  no
MKPIC:=         no
MKPROFILE:=     no

S= ${NETBSDSRCDIR}/sys
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S} -I${S}/arch

CPPFLAGS+= -D_STANDALONE

COPTS+=	-ffreestanding
COPTS+=	-fno-stack-protector
COPTS+=	-fno-unwind-tables
CWARNFLAGS+= -Werror
CWARNFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith

.if !make(obj) && !make(clean) && !make(cleandir)
.NOPATH: machine
.endif

realdepend realall: machine
CLEANFILES+= ${MACHINE_CPU} machine

machine::
	-rm -f ${MACHINE_CPU} machine
	ln -s ${S}/arch/${MACHINE}/include machine
	ln -s ${S}/arch/${MACHINE_CPU}/include ${MACHINE_CPU}

${OBJS}: machine

### find out what to use for libkern
KERN_AS=	library
KERNDIR=	${S}/lib/libkern

.include "${S}/lib/libkern/Makefile.libkern"
.ifndef ARCHSUBDIR
.BEGIN:
	@echo no ARCHSUBDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `NM=${NM} ${LORDER} ${OBJS} | ${TSORT}`

lib${LIB}.po:: ${POBJS}
	@echo building profiled ${LIB} library
	@rm -f lib${LIB}.po
	@${LD} -r -o lib${LIB}.po `NM=${NM} ${LORDER} ${POBJS} | ${TSORT}`

showsources: ${SRCS}
	@echo ${.ALLSRC}
