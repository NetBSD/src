# $NetBSD: mkvars.mk,v 1.42 2023/06/03 21:20:53 lukem Exp $

#
# Extra variables to print.
# Do not include entries from <bsd.own.mk> _MKVARS.no and _MKVAR.yes.
# Please keep alphabetically sorted with one entry per line.
#
MKEXTRAVARS= \
	ARCH64 \
	EABI \
	HAVE_ACPI \
	HAVE_BINUTILS \
	HAVE_GCC \
	HAVE_GDB \
	HAVE_LIBGCC_EH \
	HAVE_MESA_VER \
	HAVE_NVMM \
	HAVE_OPENSSL \
	HAVE_SSP \
	HAVE_UEFI \
	HAVE_XORG_GLAMOR \
	HAVE_XORG_SERVER_VER \
	KERNEL_DIR \
	MACHINE \
	MACHINE_ARCH \
	MACHINE_CPU \
	MAKEVERBOSE \
	MKCOMPAT \
	MKCOMPATMODULES \
	MKMANPAGES \
	MKSTATICPIE \
	MKXORG \
	NETBSDSRCDIR \
	OBJECT_FMT \
	TARGET_ENDIANNESS \
	TOOLCHAIN_MISSING \
	USE_INET6 \
	USE_KERBEROS \
	USE_LDAP \
	USE_YP

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
