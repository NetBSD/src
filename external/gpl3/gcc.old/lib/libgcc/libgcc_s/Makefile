#	$NetBSD: Makefile,v 1.6 2018/11/11 22:49:51 mrg Exp $

USE_SHLIBDIR=	yes
REQUIRETOOLS=	yes
NOLINT=		# defined

LIB=		gcc_s
NOSTATICLIB=	# defined
NOPROFILE=	# defined
NOPICINSTALL=	# defined

.include "../../../Makefile.gcc_path"
.include <bsd.own.mk>

.if ${MKGCC} != "no"

.cc: # disable .cc->NULL transform

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

LDFLAGS+=	-nodefaultlibs -Wl,-z -Wl,nodelete
LDFLAGS+=	-Wl,--version-script=${.OBJDIR}/libgcc.map

# XXX handle this better?  GCC 4.8 moved them.
.if ${MACHINE} == "emips"
LIB2ADD_HACK+=	${GNUHOSTDIST}/gcc/config/floatunsidf.c \
		${GNUHOSTDIST}/gcc/config/floatunsisf.c
.endif

# XXXGCC5 GCC 5.3 has all these in libgcc and we have them in libc.
.if ${MACHINE_ARCH} == "mips64el" || ${MACHINE_ARCH} == "mips64eb"
LIB2ADD_EDITED= ${G_LIB2ADD:T:Nfloatsisf.c:Nfloatunsisf.c:Nfloatsidf.c:Nfloatunsidf.c:Nfixsfsi.c:Nextendsfdf2.c:Naddsf3.c:Nmulsf3.c:Ndivsf3.c:Nadddf3.c:Nsubdf3.c:Nmuldf3.c:Ndivdf3.c:Nfixdfsi.c:S/.asm/.S/}
.else
LIB2ADD_EDITED=	${G_LIB2ADD:T:S/.asm/.S/}
.endif

SRCS+=		${LIB2FUNCS} ${LIB2DIVMOD} ${LIB2_EH} ${LIB2_EHASM} \
		${LIB2ADD_EDITED} \
		${LIB2ADD_HACK:T:S/.asm/.S/} \
		${G_LIB2ADD_ST:T} ${LIB1ASMFUNCS}

COPTS.unwind-dw2.c = -Wno-stack-protector

DPADD+=		libgcc.map
CLEANFILES+=	libgcc.map

.include <bsd.lib.mk>

.if ${MKPICLIB} != "no"
LIBGCC_S_OBJS=	${SOBJS}
.else
LIBGCC_S_OBJS=	${OBJS}
.endif

LIBGCC_STD_VER_IN=	${GNUHOSTDIST}/libgcc/libgcc-std.ver.in
CLEANFILES+=		libgcc-std.ver

libgcc-std.ver:	${LIBGCC_STD_VER_IN}
	${_MKTARGET_CREATE}
	${TOOL_SED}  -e 's/__PFX__/__/g' -e 's/__FIXPTPFX__/__/g' < ${LIBGCC_STD_VER_IN}  > ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

libgcc.map:	${LIBGCC_S_OBJS} ${G_SHLIB_MKMAP} ${G_SHLIB_MAPFILES}
	{ \
		${NM} ${G_SHLIB_NM_FLAGS} ${LIBGCC_S_OBJS}; \
		echo %%; \
		for f in ${G_SHLIB_MAPFILES}; do \
			if [ -f ${.OBJDIR}/$${f} ]; then \
				cat ${.OBJDIR}/$${f}; \
			else \
				cat $${f}; \
			fi; \
		done | \
			${TOOL_SED} -e "/^[ 	]*#/d" \
			    -e 's/^%\(if\|else\|elif\|endif\|define\)/#\1/' | \
	  		${CC} ${G_LIBGCC2_CFLAGS} ${G_INCLUDES} -E -xassembler-with-cpp -; \
	} | ${TOOL_AWK} -f ${G_SHLIB_MKMAP} > ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET} 

.else
.include <bsd.prog.mk> # do nothing
.endif
