#	$NetBSD: Makefile,v 1.8.8.2 2009/11/01 13:58:36 jym Exp $

S=	${.CURDIR}/../../../..

NOMAN=		# defined
STRIPFLAG=	# nothing

LIBCRT0=	# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing
LIBC=		# nothing

PRIMARY_LOAD_ADDRESS=0x600
SECONDARY_LOAD_ADDRESS=0x10000

PIE_CFLAGS=
PIE_LDFLAGS=
PIE_AFLAGS=

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

.if ${MACHINE_ARCH} == "x86_64"
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
	${_MKTARGET_LINK}
	${CC} -o ${PROG}.tmp ${LDFLAGS} -Wl,-Ttext,${PRIMARY_LOAD_ADDRESS} ${OBJS}
	@ set -- $$( ${NM} -t d ${PROG}.tmp | grep '\<free_space\>' \
		    | ${TOOL_SED} 's/^0*//'  ); \
		echo "#### There are $$1 free bytes in ${PROG}"
	${OBJCOPY} -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp

.include <bsd.prog.mk>
