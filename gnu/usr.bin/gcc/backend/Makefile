#	$NetBSD: Makefile,v 1.7 2002/02/26 22:30:46 tv Exp $

LIB=		backend
NOLINT=		# defined
NOPROFILE=	# defined
NOPIC=		# defined
NOLINKLIB=	# defined

SRCS=		${G_OBJS:.o=.c} ${G_out_file:T}

CPPFLAGS+=	-I. -I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DTARGET_NAME=\"${MACHINE_GNU_PLATFORM}\"
HOST_CPPFLAGS+=	-I. -I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

.include <bsd.lib.mk>

# Independent generation programs.

CLEANFILES+=	gengenrtl genrtl.c genrtl.h \
		gencheck tree-check.h

genrtl.c: genrtl.h
genrtl.h: gengenrtl.c
	${HOST_LINK.c} -o gengenrtl $>
	./gengenrtl genrtl.h genrtl.c

tree-check.h: gencheck.c
	${HOST_LINK.c} -o gencheck $>
	./gencheck >$@

.for f in attr.h attrtab.c codes.h config.h emit.c extract.c \
	flags.h opinit.c peep.c output.c recog.c
CLEANFILES+=	gen${f:R} insn-${f}

insn-${f}: genrtl.h gen${f:R}.c obstack.c ${G_md_file} \
		${G_HOST_RTL:S/^l-//:.o=.c} ${G_HOST_PRINT:S/^l-//:.o=.c}
	${HOST_LINK.c} -o gen${f:R} ${>:M*.c}
	./gen${f:R} ${G_md_file} >$@
.endfor

insn-attrtab.c: ${G_HOST_RTLANAL:S/^l-//:.o=.c}
insn-attrtab.c insn-extract.c: insn-config.h
${OBJS} .depend: insn-attr.h insn-codes.h insn-config.h insn-flags.h tree-check.h

.PATH: ${DIST}/gcc ${DIST}/libiberty ${G_out_file:H}
