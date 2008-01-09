#	$NetBSD: Makefile,v 1.24.8.1 2008/01/09 01:31:59 matt Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu

.include "${.CURDIR}/../common/Makefile.inc"
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

PROG=		ld
SRCS=		${G_OFILES:.o=.c}
CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH} \
		-I${DIST}/ld \
		-DDEFAULT_EMULATION=\"${G_EMUL}\" \
		-DSCRIPTDIR=\"/usr/share\" \
		-DTARGET=\"${G_target_alias}\" \
		-DBINDIR=\"/usr/bin\" \
		-DTOOLBINDIR=\"/usr/bin\"

LDADD=		-lintl
DPADD=		${LIBINTL}

COPTS.ldgram.c = -Wno-stack-protector

BFDOBJ!=	cd ${TOP}/${BFDSUBDIR}/libbfd && ${PRINTOBJDIR}
LDADD+=		-L${BFDOBJ} -lbfd
.if ${MKPICLIB} != "no"
DPADD+=		${BFDOBJ}/libbfd_pic.a
.else
DPADD+=		${BFDOBJ}/libbfd.a
.endif

IBERTYOBJ!=	cd ${TOP}/${BFDSUBDIR}/libiberty && ${PRINTOBJDIR}
LDADD+=		-L${IBERTYOBJ} -liberty
DPADD+=		${IBERTYOBJ}/libiberty.a

TEXINFO=	${G_TEXINFOS}
INFOFLAGS=	-I${DIST}/ld -I${DIST}/bfd/doc
.if ${MACHINE_ARCH} == "x86_64"
FILESDIR=	/usr/share/ldscripts
.endif

.PATH: ${DIST}/ld ${DIST}/ld/emulparams \
	${DIST}/ld/emultempl ${DIST}/ld/scripttempl ldscripts

BUILDSYMLINKS+=	${G_STRINGIFY} stringify.sed
BUILDSYMLINKS+=	gen-doc.texi configdoc.texi

${PROG}.info: configdoc.texi

.for f in ${G_EMULATION_OFILES:S/^e//:R}
.if exists(.depend.${f})
.include ".depend.${f}"
.endif

.depend.${f}: ${f}.sh
	${_MKTARGET_CREATE}
	(srcdir=${DIST}/ld EMULATION_NAME=$f . ${>:M*.sh} && \
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

DPSRCS+=	.depend.${f} e${f}.c
CLEANFILES+=	.depend.${f} e${f}.c

e${f}.c: ${DIST}/ld/genscripts.sh ${.CURDIR}/Makefile stringify.sed
	${_MKTARGET_CREATE}
	unset MACHINE || true; \
	LIB_PATH=/usr/lib
		${HOST_SH} ${DIST}/ld/genscripts.sh ${DIST}/ld ${LIBDIR} "/usr" \
		${G_target_alias} ${G_target_alias} ${G_target_alias} \
		${G_EMUL} ${LIBDIR} no ${f} "${G_target_alias}"

.if ${MACHINE_ARCH} == "x86_64"
FILES+=		${f}.x ${f}.xbn ${f}.xn ${f}.xr ${f}.xu

# XXX hack to find out if .xs exists - slow!
HAS_XS!=	grep '^GENERATE_SHLIB_SCRIPT' ${DIST}/ld/emulparams/${f}.sh || echo
.if !empty(HAS_XS)
FILES+= ${f}.xc ${f}.xd ${f}.xdc ${f}.xdw ${f}.xs ${f}.xsc ${f}.xsw ${f}.xw
.endif
.endif
.endfor

.include <bsd.prog.mk>
.include <bsd.info.mk>

# Make sure we use the pre-generated C files
.l.c .y.c .y.h:
	@true

cleanprog: __cleanldscripts 
__cleanldscripts:
	-rm -rf ldscripts tmpdir
