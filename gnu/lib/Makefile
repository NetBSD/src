#	$NetBSD: Makefile,v 1.22 2000/05/10 22:39:29 thorpej Exp $

HAVE_GCC29!=	${CXX} --version | egrep "^(2\.9)" ; echo

SUBDIR+= libbfd libmalloc

.if empty(HAVE_GCC29) || make (obj) || make(cleandir) || make(clean)
SUBDIR+= libg2c libgcc libobjc libstdc++
.endif

.include <bsd.subdir.mk>
