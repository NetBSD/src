#	$NetBSD: Makefile.inc,v 1.3 1998/04/02 06:33:31 mrg Exp $

.if !defined(GCC_MAKEFILE_INC)
GCC_MAKEFILE_INC=1

DIST=	$(.CURDIR)/../../../dist/gcc
COMMON=	$(.CURDIR)/../common
COMMONOBJ!=cd $(.CURDIR)/../common; \
	 printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

CPPFLAGS+=-I$(.CURDIR) \
	-I$(COMMON) \
	-I$(COMMONOBJ) \
	-I$(.CURDIR)/../arch \
	-I$(.CURDIR)/../arch/$(MACHINE_ARCH) \
	-I$(DIST) \
	-I$(DIST)/config \
	-I$(DIST)/config/$(MACHINE_ARCH) \
	-DMD_EXEC_PREFIX=\"/usr/libexec/\" \
	-DMD_STARTFILE_PREFIX=\"/usr/lib/\" \
	-DIN_GCC -DNETBSD_NATIVE

LDADD+=		-lgnumalloc
DPADD+=		$(LIBGNUMALLOC)

.PATH: $(DIST) $(DIST)/config $(DIST)/config/$(MACHINE_ARCH) \
	$(COMMON) $(COMMONOBJ)

# XXX
version=egcs-2.90.27
target_alias=${MACHINE_ARCH}-netbsd
CPPFLAGS+= \
	-DDEFAULT_TARGET_VERSION=\"$(version)\" \
	-DDEFAULT_TARGET_MACHINE=\"$(target_alias)\" \
# XXX

.endif
