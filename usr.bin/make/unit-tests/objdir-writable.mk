# $NetBSD: objdir-writable.mk,v 1.1 2020/11/12 23:35:21 sjg Exp $

# test checking for writable objdir

RO_OBJDIR?= ${TMPDIR:U/tmp}/roobj

.if make(do-objdir)
# this should succeed
.OBJDIR: ${RO_OBJDIR}

do-objdir:
.else
all: no-objdir ro-objdir explicit-objdir

# make it now
x!= echo; mkdir -p ${RO_OBJDIR};  chmod 555 ${RO_OBJDIR}

no-objdir:
	@MAKEOBJDIR=${RO_OBJDIR} ${.MAKE} -r -f /dev/null -C /tmp -V .OBJDIR

ro-objdir:
	@MAKEOBJDIR=${RO_OBJDIR} ${.MAKE} -r -f /dev/null -C /tmp -V .OBJDIR MAKE_OBJDIR_CHECK_WRITABLE=no

explicit-objdir:
	@${.MAKE} -r -f ${MAKEFILE} -C /tmp do-objdir -V .OBJDIR
.endif

