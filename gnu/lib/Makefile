#	$NetBSD: Makefile,v 1.29 2001/12/31 23:04:12 thorpej Exp $

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

.if ${MKBFD} != "no"
SUBDIR+= libbfd libiberty
.endif

.if ${MKGCC} != "no"
SUBDIR+= libg2c libgcc libobjc libstdc++
.endif

SUBDIR+= libmalloc

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
