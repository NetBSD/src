#	$NetBSD: Makefile,v 1.23 2001/05/10 12:53:02 mason Exp $

HAVE_GCC29!=	${CXX} --version | egrep "^(2\.9)" ; echo

SUBDIR+= libbfd libmalloc

.if empty(HAVE_GCC29) || make (obj) || make(cleandir) || make(clean)
SUBDIR+= libg2c libgcc libobjc
.if ${MACHINE_ARCH} != "sparc64"
SUBDIR+= libstdc++
.endif
.endif

.include <bsd.subdir.mk>
