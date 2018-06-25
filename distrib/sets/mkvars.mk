# $NetBSD: mkvars.mk,v 1.34.2.1 2018/06/25 07:25:09 pgoyette Exp $

MKEXTRAVARS= \
	MACHINE \
	MACHINE_ARCH \
	MACHINE_CPU \
	HAVE_GCC \
	HAVE_GDB \
	HAVE_XORG_SERVER_VER \
	HAVE_BINUTILS \
	HAVE_LIBGCC_EH \
	HAVE_OPENSSL \
	HAVE_SSP \
	OBJECT_FMT \
	TOOLCHAIN_MISSING \
	EXTSRCS \
	MKMANZ \
	MKBFD \
	MKCOMPAT \
	MKCOMPATTESTS \
	MKCOMPATMODULES \
	MKDTC \
	MKDYNAMICROOT \
	MKFIRMWARE \
	MKMANPAGES \
	MKSLJIT \
	MKSOFTFLOAT \
	MKXORG \
	MKXORG_SERVER \
	MKRADEONFIRMWARE \
	MKSTATICPIE \
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

.if ${MKKMOD} != "no" && ${MKCOMPATMODULES} != "no"
ARCHDIR_SUBDIR:=
.include "${NETBSDSRCDIR}/sys/modules/arch/archdirs.mk"
KMODARCHDIRS:=${ARCHDIR_SUBDIR:T}
.endif

.if ${MKX11} != "no"
MKXORG:=yes
# We have to force this off, because "MKX11" is still an option
# that is in _MKVARS.
MKX11:=no
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
	@echo COMPATARCHDIRS=${COMPATARCHDIRS:S/ /,/gW}
.else
	@echo COMPATARCHDIRS=
.endif
.if ${MKKMOD} != "no" && ${MKCOMPATMODULES} != "no"
	@echo KMODARCHDIRS=${KMODARCHDIRS:S/ /,/gW}
.else
	@echo KMODARCHDIRS=
.endif

mksolaris: .PHONY
.if (${MKDTRACE} != "no" || ${MKZFS} != "no" || ${MKCTF} != "no")
	@echo MKSOLARIS="yes"
.else
	@echo MKSOLARIS="no"
.endif

.include <bsd.files.mk>
