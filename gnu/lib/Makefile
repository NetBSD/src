#	$NetBSD: Makefile,v 1.21 2000/05/09 06:23:40 thorpej Exp $

HAVE_GCC29!=	${CXX} --version | egrep "^(2\.9|egcs)" ; echo

SUBDIR+= libbfd libmalloc

.if empty(HAVE_GCC29) || make (obj) || make(cleandir) || make(clean)
SUBDIR+= libg2c libgcc libobjc libstdc++
.endif

.include <bsd.subdir.mk>
