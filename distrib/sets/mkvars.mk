# $NetBSD: mkvars.mk,v 1.10 2013/04/30 20:54:22 matt Exp $

MKEXTRAVARS= \
	MACHINE \
	MACHINE_ARCH \
	MACHINE_CPU \
	HAVE_GCC \
	HAVE_GDB \
	HAVE_SSP \
	OBJECT_FMT \
	TOOLCHAIN_MISSING \
	EXTSRCS \
	MKMANZ \
	MKBFD \
	MKCOMPAT \
	MKCOMPATMODULES \
	MKDYNAMICROOT \
	MKMANPAGES \
	MKSLJIT \
	MKSOFTFLOAT \
	MKXORG \
	X11FLAVOR \
	USE_INET6 \
	USE_KERBEROS \
	USE_LDAP \
	USE_YP \
	NETBSDSRCDIR \
	MAKEVERBOSE \
	TARGET_ENDIANNESS \
	EABI \
	ARCH64

#####

.include <bsd.own.mk>
.include <bsd.sys.mk>
.include <bsd.endian.mk>

.if (${MKMAN} == "no" || empty(MANINSTALL:Mmaninstall))
MKMANPAGES=no
.else
MKMANPAGES=yes
.endif

.if ${MKX11} != "no"
. if ${X11FLAVOUR} == "Xorg"
MKXORG:=yes
MKX11:=no
. else
MKXORG:=no
. endif
.endif

.if (!empty(MACHINE_ARCH:Mearm*))
EABI=yes
.else
EABI=no
.endif

.if (!empty(MACHINE_ARCH:M*64*) || ${MACHINE_ARCH} == alpha)
ARCH64=yes
.else
ARCH64=no
.endif

#####

mkvars: mkvarsyesno mkextravars mksolaris .PHONY

mkvarsyesno: .PHONY
.for i in ${_MKVARS.yes}
	@echo $i="${$i}"
.endfor
.for i in ${_MKVARS.no}
	@echo $i="${$i}"
.endfor

mkextravars: .PHONY
.for i in ${MKEXTRAVARS}
	@echo $i="${$i}"
.endfor

mksolaris: .PHONY
.if (${MKDTRACE} != "no" || ${MKZFS} != "no")
	@echo MKSOLARIS="yes"
.else
	@echo MKSOLARIS="no"
.endif
