#	$NetBSD: Makefile,v 1.10 2003/03/01 13:09:20 mrg Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/toolchain
GNUHOSTDIST=	${DIST}

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

PROG=		ld
SRCS=		${G_OFILES:.o=.c}
CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH} \
		-DDEFAULT_EMULATION=\"${G_EMUL}\" \
		-DSCRIPTDIR=\"/usr/share\" \
		-DTARGET=\"${G_target_alias}\"

LDADD=		-lintl
DPADD=		${LIBINTL}

BFDOBJ!=	cd ${TOP}/lib/libbfd && ${PRINTOBJDIR}
LDADD+=		-L${BFDOBJ} -lbfd
DPADD+=		${BFDOBJ}/libbfd_pic.a

IBERTYOBJ!=	cd ${TOP}/lib/libiberty && ${PRINTOBJDIR}
LDADD+=		-L${IBERTYOBJ} -liberty
DPADD+=		${IBERTYOBJ}/libiberty.a

TEXINFO=	${G_TEXINFOS}
INFOFLAGS=	-I${DIST}/ld -I${DIST}/bfd/doc
#FILESDIR=	/usr/share/ldscripts

.PATH: ${DIST}/ld ${DIST}/ld/emulparams \
	${DIST}/ld/emultempl ${DIST}/ld/scripttempl ldscripts

CLEANFILES+=	stringify.sed
stringify.sed: ${G_STRINGIFY}
	cp $> $@

.for f in ${G_EMULATION_OFILES:S/^e//:R}
.if exists(.depend.${f})
.include ".depend.${f}"
.endif

.depend: .depend.${f}
.depend.${f}: ${f}.sh
	(srcdir=${DIST}/ld . ${>:M*.sh} && \
		if [ x"$$TEMPLATE_NAME" != x ]; then \
			temp_emul=$$TEMPLATE_NAME.em; \
		else \
			temp_emul=; \
		fi; \
		if [ x"$$SCRIPT_NAME" != x ]; then \
			script=$$SCRIPT_NAME.sc; \
		else \
			script=; \
		fi; \
		echo "e${f}.c: $$temp_name $$script" \
	) >$@

CLEANFILES+=	.depend.${f} e${f}.c

e${f}.c: ${DIST}/ld/genscripts.sh ${.CURDIR}/Makefile stringify.sed
	unset MACHINE || true; \
	LIB_PATH=/usr/lib srcdir=${DIST}/ld \
		sh ${DIST}/ld/genscripts.sh ${DIST}/ld ${LIBDIR} /usr \
		${G_target_alias} ${G_target_alias} ${G_target_alias} \
		${G_EMUL} ${LIBDIR} ${f}

#FILES+=		${f}.x ${f}.xbn ${f}.xn ${f}.xr ${f}.xu

# XXX hack to find out if .xs exists - slow!
#HAS_XS!=	grep '^GENERATE_SHLIB_SCRIPT' ${DIST}/ld/emulparams/${f}.sh || echo
#.if !empty(HAS_XS)
#FILES+=		${f}.xs
#.endif
.endfor

.include <bsd.prog.mk>
.include <bsd.info.mk>

# Make sure we use the pre-generated C files
.l.c .y.c .y.h:
	@true

cleanprog: __cleanldscripts 
__cleanldscripts:
	-rm -rf ldscripts
