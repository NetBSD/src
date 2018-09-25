# $NetBSD: Makefile,v 1.5 2018/09/25 12:34:42 joerg Exp $

PROG=	xz
BINDIR=	/usr/bin
SRCS=	args.c coder.c file_io.c hardware.c list.c main.c \
	message.c options.c signals.c suffix.c util.c \
	tuklib_exit.c tuklib_cpucores.c tuklib_open_stdxxx.c \
	tuklib_mbstr_fw.c tuklib_mbstr_width.c \
	tuklib_physmem.c tuklib_progname.c mytime.c

USETBL=	yes

.include <bsd.init.mk>

CPPFLAGS+=	-DPACKAGE=\"xz\"
.if !defined(HOSTPROG)
CPPFLAGS+=	-DENABLE_NLS -DLOCALEDIR=\"/usr/share/locale\"
.endif

.if !defined(HOSTPROG)
LINKS+=	${BINDIR}/xz ${BINDIR}/lzcat
LINKS+=	${BINDIR}/xz ${BINDIR}/lzma
LINKS+=	${BINDIR}/xz ${BINDIR}/unlzma
LINKS+=	${BINDIR}/xz ${BINDIR}/unxz
LINKS+=	${BINDIR}/xz ${BINDIR}/xzcat
.endif

MLINKS+=	xz.1 lzcat.1
MLINKS+=	xz.1 lzma.1
MLINKS+=	xz.1 unlzma.1
MLINKS+=	xz.1 unxz.1
MLINKS+=	xz.1 xzcat.1

.PATH:	${XZSRCDIR}/src/common ${XZSRCDIR}/src/xz

.if ${MKNLS} != "no"
.for lang in cs de it
FILES+=					${XZSRCDIR}/po/${lang}.gmo
FILESDIR_${XZSRCDIR}/po/${lang}.gmo=	/usr/share/locale/${lang}/LC_MESSAGES
FILESNAME_${XZSRCDIR}/po/${lang}.gmo=	xz.mo
.endfor
.endif

.if defined(HOSTPROG)
HOST_CPPFLAGS+=	${CPPFLAGS:N-Wp,-iremap,*}
XZLIBDIR!=	cd ${NETBSDSRCDIR}/tools/xz-lib && ${PRINTOBJDIR}
LDADD+=	-L${XZLIBDIR} -llzma
DPADD+=	${XZLIBDIR}/liblzma.a
.else
DPADD+=	${LIBLZMA} ${LIBINTL} ${LIBPTHREAD}
LDADD+=	-llzma -lintl -lpthread
.include <bsd.prog.mk>
.endif
