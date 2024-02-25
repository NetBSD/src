#	$NetBSD: Makefile,v 1.5.6.1 2024/02/25 15:42:53 martin Exp $

LDADD+=-lns
DPADD+=${LIBNS}

.include <bsd.init.mk>

PROG=	named
MAN=	named.8 named.conf.5
BINDIR=	/usr/sbin

DIST=${IDIST}/bin/named
CPPFLAGS+=-I${DIST}/include -I${DIST}/unix/include
CPPFLAGS+=-I${LIBUVDIR}/dist/include
CPPFLAGS+=-DNO_VERSION_DATE
CPPFLAGS+=-DNAMED_SYSCONFDIR=\"${SYSCONFDIR}\"
CPPFLAGS+=-DNAMED_LOCALSTATEDIR=\"/var\"

.if defined(HAVE_GCC) || defined(HAVE_LLVM)
.for f in client
COPTS.${f}.c+=  -fno-strict-aliasing
.endfor
.endif

CWARNFLAGS.clang+=	-Wno-tautological-constant-out-of-range-compare

.PATH: ${DIST}/unix ${DIST} ${IDIST}/doc/man

SRCS_UNIX=	os.c dlz_dlopen_driver.c
SRCS=	builtin.c config.c control.c controlconf.c \
	log.c logconf.c \
	main.c server.c statschannel.c \
	tkeyconf.c transportconf.c tsigconf.c \
	xsl.c zoneconf.c ${SRCS_UNIX}


xsl.c: bind9.xsl
	${MKTARGET_CREATE}
	(echo 'const char xslmsg[] =' &&	\
	 ${TOOL_SED}	-e 's,\",\\\",g'	\
		-e 's,^,\",'			\
		-e 's,$$,\\n\",' &&		\
	 echo ";")				\
	< ${.ALLSRC} > ${.TARGET}

CLEANFILES+= xsl.c

main.o: ${VERSIONFILE}


.include <bsd.prog.mk>
