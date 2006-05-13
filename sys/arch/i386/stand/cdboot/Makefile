#	$NetBSD: Makefile,v 1.4 2006/05/13 09:13:11 lukem Exp $

S=	${.CURDIR}/../../../..

NOMAN=		# defined
STRIPFLAG=	# nothing

LIBCRT0=	# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing
LIBC=		# nothing

PRIMARY_LOAD_ADDRESS=0x600
SECONDARY_LOAD_ADDRESS=0x10000

.include <bsd.own.mk>

PROG=	cdboot
SRCS?=	cdboot.S

BINDIR=	/usr/mdec
BINMODE=444

.PATH:	${.CURDIR}/..

LDFLAGS+= -nostdlib -Wl,-e,start
CPPFLAGS+= -I. -I${.CURDIR}/../lib -I${S}
CPPFLAGS+= -DPRIMARY_LOAD_ADDRESS=${PRIMARY_LOAD_ADDRESS}
CPPFLAGS+= -DSECONDARY_LOAD_ADDRESS=${SECONDARY_LOAD_ADDRESS}
#CPPFLAGS+= -DDISABLE_KEYPRESS

.if ${MACHINE} == "amd64"
LDFLAGS+=  -Wl,-m,elf_i386
AFLAGS+=   -m32
.endif

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN: machine x86
.NOPATH: machine x86
.endif

realdepend realall: machine x86
CLEANFILES+= machine x86

machine::
	-rm -f $@
	ln -s $S/arch/i386/include $@

x86::
	-rm -f $@
	ln -s $S/arch/x86/include $@

${OBJS}: machine x86

CLEANFILES+= ${PROG}.tmp

${PROG}: ${OBJS}
	${CC} -o ${PROG}.tmp ${LDFLAGS} -Ttext ${PRIMARY_LOAD_ADDRESS} ${OBJS}
	@ set -- $$( ${NM} -t d ${PROG}.tmp | grep '\<free_space\>' | sed 's/^0*//'  ); \
		echo "#### There are $$1 free bytes in ${PROG}"
	${OBJCOPY} -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp

.include <bsd.prog.mk>
