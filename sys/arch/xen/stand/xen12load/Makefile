# $NetBSD: Makefile,v 1.1 2004/04/17 23:20:37 cl Exp $

S=	${.CURDIR}/../../../../

TEXTADDR?=	c0100000

NOMAN=
PROG?=		xen12load

SOURCES?=	start.S xenload.c loadelf.c
SRCS= ${SOURCES}

.include <bsd.own.mk>

LIBCRT0=	# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing
LIBC=		# nothing

BINDIR=		/usr/mdec
BINMODE=	444

STRIP=

.PATH:	${.CURDIR}/.. ${.CURDIR}/../../lib

LDFLAGS+= -N -e load_start
CPPFLAGS+= -I ${S}/lib/libsa
CPPFLAGS+= -I ${.OBJDIR}/xen-ma -I ${.OBJDIR}

# Make sure we override any optimization options specified by the user
COPTS=  -Os

COPTS+=    -mcpu=i386

COPTS+=    -ffreestanding
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes
CFLAGS+= -fPIC
CPPFLAGS+= -nostdinc -D_STANDALONE
CPPFLAGS+= -I$S


XEN_STAND_DIR?= $S/arch/xen/stand

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN: machine i386 x86 lib
.NOPATH: machine i386 x86
.endif

realdepend realall: machine xen-ma i386 x86 lib
CLEANFILES+= machine i386 x86

machine::
	-rm -f $@
	ln -s $S/arch/xen/include $@

xen-ma::
	-rm -rf $@
	mkdir $@
	ln -s ../i386 $@/machine

i386::
	-rm -f $@
	ln -s $S/arch/i386/include $@

x86::
	-rm -f $@
	ln -s $S/arch/x86/include $@

${OBJS}: machine i386 x86 lib

lib:
.ifdef LIBOBJ
	-rm -f $@
	ln -s ${LIBOBJ}/lib .
	[ -d ${LIBOBJ}/lib ] || mkdir ${LIBOBJ}/lib
.else
	mkdir lib
.endif

### find out what to use for libsa
SA_AS= library
SAMISCMAKEFLAGS+="SA_USE_LOADFILE=no"
SAMISCMAKEFLAGS+="SA_INCLUDE_NET=no"
SAMISCMAKEFLAGS+="SA_USE_CREAD=no"
.include "${S}/lib/libsa/Makefile.inc"
LIBSA= ${SALIB}

cleandir distclean: cleanlibdir cleanxenmadir

cleanlibdir:
	rm -rf lib

cleanxenmadir:
	rm -rf xen-ma

LIBLIST= ${LIBSA}

CLEANFILES+= ${PROG}.tmp ${PROG}.tmp2

AWKPROG='\
	function x(v) { \
		printf "\\%o\\%o\\%o\\%o", v % 256, (v / 2^8) % 256, \
		    (v / 2^16) % 256, (v / 2^24) % 256 } \
	{ \
		printf "XenoGues"; \
		x($$1); \
	}'

${PROG}: ${OBJS} ${LIBLIST}
	${_MKTARGET_LINK}
	${LD} -o ${PROG}.tmp ${LDFLAGS} -Ttext 0x${TEXTADDR} \
		${OBJS} ${LIBLIST}
	printf $$(printf %d 0x${TEXTADDR} | awk ${AWKPROG}) >${PROG}
	${OBJCOPY} -O binary -R .ident -R .comment ${PROG}.tmp ${PROG}.tmp2
	cat ${PROG}.tmp2 >>${PROG}
	rm -f ${PROG}.tmp ${PROG}.tmp2

.include <bsd.prog.mk>
