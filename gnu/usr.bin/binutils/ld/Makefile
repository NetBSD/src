#	$NetBSD: Makefile,v 1.2 2001/08/06 19:34:22 tv Exp $

TOP=		${.CURDIR}/../../..
DIST=		${TOP}/dist/toolchain

.include <bsd.own.mk>
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

TARGET=		${G_target_alias:C/[\.0-9]*$//}

PROG=		ld
SRCS=		${G_OFILES:.o=.c}
CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH} \
		-DDEFAULT_EMULATION=\"${G_EMUL}\" \
		-DSCRIPTDIR=\"/usr/share\" \
		-DTARGET=\"${TARGET}\"

LDADD=		-lintl
DPADD=		${LIBINTL}

BFDOBJ!=	cd ${TOP}/lib/libbfd && ${PRINTOBJDIR}
LDADD+=		-L${BFDOBJ} -lbfd
DPADD+=		${BFDOBJ}/libbfd_pic.a

IBERTYOBJ!=	cd ${TOP}/lib/libiberty && ${PRINTOBJDIR}
LDADD+=		-L${IBERTYOBJ} -liberty
DPADD+=		${IBERTYOBJ}/libiberty.a

TEXINFO=	ld.texinfo
INFOFLAGS=	-I${DIST}/ld -I${DIST}/bfd/doc
FILESDIR=	/usr/share/ldscripts

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
	(. ${>:M*.sh} && echo "e${f}.c: $$TEMPLATE_NAME.em $$SCRIPT_NAME.sc") >$@

CLEANFILES+=	.depend.${f} e${f}.c

e${f}.c: ${DIST}/ld/genscripts.sh stringify.sed
	LIB_PATH=/usr/lib sh ${DIST}/ld/genscripts.sh ${DIST}/ld ${LIBDIR} /usr \
		none ${TARGET} ${TARGET} ${G_EMUL} ${LIBDIR} ${f}

FILES+=		${f}.x ${f}.xbn ${f}.xn ${f}.xr ${f}.xu

# XXX hack to find out if .xs exists - slow!
HAS_XS!=	grep '^GENERATE_SHLIB_SCRIPT' ${DIST}/ld/emulparams/${f}.sh || echo
.if !empty(HAS_XS)
FILES+=		${f}.xs
.endif
.endfor

.include <bsd.prog.mk>
.include <bsd.info.mk>

cleanprog: __cleanldscripts 
__cleanldscripts:
	-rm -rf ldscripts
