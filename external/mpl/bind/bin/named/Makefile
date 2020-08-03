#	$NetBSD: Makefile,v 1.4 2020/08/03 17:23:35 christos Exp $

LDADD+=-lns
DPADD+=${LIBNS}

.include <bsd.init.mk>

PROG=	named
MAN=	named.8 named.conf.5
BINDIR=	/usr/sbin

DIST=${IDIST}/bin/named
CPPFLAGS+=-I${DIST}/include -I${DIST}/unix/include -DCONFIGARGS=\"defaults\"
CPPFLAGS+=-DNO_VERSION_DATE -DPRODUCT=\"BIND\" -DSRCID=\"${SRCID}\"
CPPFLAGS+=-DDESCRIPTION=\"\(Extended\ Support\ Version\)\"
CPPFLAGS+=-DBUILDER=\"make\"	# I am tempted to say Bob
CPPFLAGS+=-DNAMED_SYSCONFDIR=\"${SYSCONFDIR}\"
CPPFLAGS+=-DNAMED_LOCALSTATEDIR=\"/var\"

.include "${IDIST}/srcid"

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
	tkeyconf.c tsigconf.c \
	zoneconf.c ${SRCS_UNIX}

main.o: ${VERSIONFILE}

.include <bsd.prog.mk>
