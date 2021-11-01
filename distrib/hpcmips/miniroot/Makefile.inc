#	$NetBSD: Makefile.inc,v 1.22 2021/11/01 19:37:32 martin Exp $

DBG=		-Os

IMAGESIZE=	4352k
MAKEFS_FLAGS+=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all ipty
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${DESTDIR}/.profile
SMALLPROG_INET6=1

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
HACK_CURSES=yes
.MAKEFLAGS+=	USE_WIDECHAR=no
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
