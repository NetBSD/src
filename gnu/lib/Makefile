#	$NetBSD: Makefile,v 1.22.2.1 2001/05/15 21:44:28 he Exp $

HAVE_GCC29!=	${CXX} --version | egrep "^(2\.9)" ; echo

SUBDIR+= libbfd libmalloc

.if empty(HAVE_GCC29) || make (obj) || make(cleandir) || make(clean)
SUBDIR+= libg2c libgcc libobjc
.if ${MACHINE_ARCH} != "sparc64"
SUBDIR+= libstdc++
.endif
.endif

.include <bsd.subdir.mk>
