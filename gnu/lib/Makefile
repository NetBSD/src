#	$NetBSD: Makefile,v 1.27 2001/12/15 03:37:36 thorpej Exp $

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

.if !defined(BOOTSTRAP_NEW_TOOLCHAIN)
SUBDIR+= libbfd libg2c libgcc libiberty libobjc libmalloc libstdc++
.endif

.else
#
# XXX - BEGIN - Old toolchain build goop - delete when everything migrated - XXX
#
HAVE_GCC29!=	${CXX} --version | egrep "^(2\.9)" ; echo

SUBDIR+= libbfd libmalloc

.if empty(HAVE_GCC29) || make (obj) || make(cleandir) || make(clean)
SUBDIR+= libg2c libgcc libobjc
.if (${MACHINE_ARCH} != "sparc64") && (${MACHINE_ARCH} != "arm")
SUBDIR+= libstdc++
.endif
.endif
#
# XXX - END - Old toolchain build goop - delete when everything migrated - XXX
#
.endif

.include <bsd.subdir.mk>
