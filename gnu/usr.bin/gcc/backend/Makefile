#	$NetBSD: Makefile,v 1.3 2001/07/24 21:16:08 tv Exp $

LIB=		backend
MKLINT=		no
MKPROFILE=	no
MKPIC=		no
MKLINKLIB=	no

SRCS=		${G_OBJS:.o=.c} ${G_out_file:T}

CPPFLAGS+=	-I. -I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DTARGET_NAME=\"${MACHINE_GNU_ARCH}-netbsd\"
HOST_CPPFLAGS+=	-I. -I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

.include <bsd.lib.mk>

# Independent generation programs.

CLEANFILES+=	gengenrtl genrtl.c genrtl.h \
		gencheck tree-check.h

genrtl.c genrtl.h: gengenrtl.c
	${HOST_LINK.c} -o gengenrtl $>
	./gengenrtl genrtl.h genrtl.c

tree-check.h: gencheck.c
	${HOST_LINK.c} -o gencheck $>
	./gencheck >$@

# Programs which depend on common sources.  The ${MAKE} dance
# ensures that these programs regen their object files
# whenever they are needed (so a build can be restarted on
# another platform if desired).

LOFILES=	obstack.lo ${G_HOST_RTL:.o=.lo} ${G_HOST_PRINT:.o=.lo}
lofiles: ${LOFILES}

.if !target(.BEGIN) && !make(lofiles)
.BEGIN:
	@rm -f ${LOFILES}
.endif

.for f in attr.h attrtab.c codes.h config.h emit.c extract.c \
	flags.h opinit.c peep.c output.c recog.c
CLEANFILES+=	gen${f:R} insn-${f}

insn-${f}: genrtl.h gen${f:R}.c ${G_md_file}
	@cd ${.CURDIR} && ${MAKE} lofiles
	${HOST_LINK.c} -o gen${f:R} ${>:M*.c} ${LOFILES}
	./gen${f:R} ${G_md_file} >$@
.endfor

insn-attrtab.c: ${G_HOST_RTLANAL:.o=.c}
insn-extract.c: insn-config.h
${OBJS} .depend: insn-attr.h insn-codes.h insn-config.h insn-flags.h tree-check.h

.PATH: ${DIST}/gcc ${DIST}/libiberty ${G_out_file:H}
