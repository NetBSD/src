# $NetBSD: mkvars.mk,v 1.20 2015/06/22 08:51:42 martin Exp $

MKEXTRAVARS= \
	MACHINE \
	MACHINE_ARCH \
	MACHINE_CPU \
	HAVE_GCC \
	HAVE_GDB \
	HAVE_LIBGCC_EH \
	HAVE_SSP \
	OBJECT_FMT \
	TOOLCHAIN_MISSING \
	EXTSRCS \
	MKMANZ \
	MKBFD \
	MKCOMPAT \
	MKCOMPATTESTS \
	MKCOMPATMODULES \
	MKDYNAMICROOT \
	MKMANPAGES \
	MKSLJIT \
	MKSOFTFLOAT \
	MKXORG \
	MKXORG_SERVER \
	MKRADEONFIRMWARE \
	X11FLAVOR \
	USE_INET6 \
	USE_KERBEROS \
	USE_LDAP \
	USE_YP \
	NETBSDSRCDIR \
	MAKEVERBOSE \
	TARGET_ENDIANNESS \
	EABI \
	ARCH64 \
	COMPATARCHDIRS \
	KMODARCHDIRS

#####

.include <bsd.own.mk>
.include <bsd.endian.mk>

.if (${MKMAN} == "no" || empty(MANINSTALL:Mmaninstall))
MKMANPAGES=no
.else
MKMANPAGES=yes
.endif

.if ${MKCOMPAT} != "no"
ARCHDIR_SUBDIR:=
.include "${NETBSDSRCDIR}/compat/archdirs.mk"
COMPATARCHDIRS:=${ARCHDIR_SUBDIR:T}
.endif

.if ${MKKMOD} != "no"
ARCHDIR_SUBDIR:=
.include "${NETBSDSRCDIR}/sys/modules/arch/archdirs.mk"
KMODARCHDIRS:=${ARCHDIR_SUBDIR:T}
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
.if ${MKCOMPAT} != "no"
	@echo COMPATARCHDIRS=${COMPATARCHDIRS} | ${TOOL_SED} -e's/ /,/'
.endif
.if ${MKKMOD} != "no"
	@echo KMODARCHDIRS=${KMODARCHDIRS} | ${TOOL_SED} -e's/ /,/'
.endif

mksolaris: .PHONY
.if (${MKDTRACE} != "no" || ${MKZFS} != "no")
	@echo MKSOLARIS="yes"
.else
	@echo MKSOLARIS="no"
.endif

.include <bsd.files.mk>
