#	$NetBSD: Makefile,v 1.3.14.1 1999/12/27 18:34:18 wrstuden Exp $

LIB=	dos

MKLINT=no
MKPROFILE=no
MKPIC=no

CPPFLAGS+=	-I${.CURDIR}

AWK?=	awk

SRCS=	dos_rename.S
SRCS+=	dos_cerror.S dos_procerr.S dos_errno.c dos_strerror.S
CLEANFILES+=	dos_strerror.S

ASM=	exit.o getchar.o putchar.o cominp.o comout.o prnout.o inpout.o \
	inkey.o getc.o print.o gets.o keysns.o kflushgp.o kflushio.o \
	kflushin.o kflushgc.o kflushgs.o fflush.o chgdrv.o drvctrl.o \
	consns.o prnsns.o cinsns.o coutsns.o fatchk.o fatchk2.o hendspmo.o \
	hendspmp.o hendspmr.o hendspmc.o hendspio.o hendspip.o hendspir.o \
	hendspic.o hendspso.o hendspsp.o hendspsr.o hendspsc.o curdrv.o \
	getss.o fgetc.o fgets.o fputc.o fputs.o allclose.o super.o fnckeygt.o \
	fnckeyst.o c_putc.o c_print.o c_color.o c_locate.o c_down_s.o \
	c_up_s.o c_up.o c_down.o c_right.o c_left.o c_cls_ed.o c_cls_st.o \
	c_cls_al.o c_era_ed.o c_era_st.o c_era_al.o c_ins.o c_del.o \
	c_fnkmod.o c_window.o c_width.o c_curon.o c_curoff.o k_keyinp.o \
	k_keysns.o k_sftsns.o k_keybit.o k_insmod.o intvcs.o pspset.o \
	gettim2.o settim2.o namests.o getdate.o setdate.o gettime.o settime.o \
	verify.o dup0.o vernum.o keeppr.o getdpb.o breakck.o drvxchg.o \
	intvcg.o dskfre.o nameck.o mkdir.o rmdir.o chdir.o create.o open.o \
	close.o read.o write.o delete.o seek.o chmod.o ioctrlgt.o ioctrlst.o \
	ioctrlrh.o ioctrlwh.o ioctrlrd.o ioctrlwd.o ioctrlis.o ioctrlos.o \
	ioctrldvgt.o ioctrlfdgt.o ioctrlrtset.o ioctrldvctl.o ioctrlfdctl.o \
	dup.o dup2.o curdir.o malloc.o mfree.o setblock.o loadexec.o load.o \
	pathchk.o loadonly.o execonly.o bindno.o exec2.o exit2.o wait.o \
	files.o exfiles.o nfiles.o exnfiles.o setpdb.o getpdb.o setenv.o \
	getenv.o verifyg.o common_ck.o common_rd.o common_wt.o common_lk.o \
	common_fre.o common_del.o move.o filedate.o malloc2.o malloc0.o \
	maketmp.o newfile.o lock.o unlock.o getassign.o makeassign.o \
	rassign.o fflush_set.o os_patch.o get_fcb_adr.o s_malloc.o \
	s_malloc0.o s_mfree.o s_process.o retshell.o ctlabort.o errabort.o \
	diskred.o diskred2.o diskwrt.o diskwrt2.o indosflg.o super_jsr.o \
	memcpy.o open_pr.o kill_pr.o get_pr.o suspend_pr.o sleep_pr.o \
	send_pr.o time_pr.o change_pr.o

OBJS+=	${ASM}

MAKECALL=${.CURDIR}/makedoscalls.awk

${ASM}: ${.CURDIR}/dos.h ${MAKECALL}
	@echo creating ${.TARGET}
	@grep -i 'DOS_${.PREFIX} ' ${.CURDIR}/dos.h | \
	    ${AWK} -f ${MAKECALL} | ${CPP} ${CPPFLAGS} | ${AS} -o ${.TARGET}

MAKESTRERR=${.CURDIR}/makestrerror.awk

dos_strerror.S: ${.CURDIR}/dos_errno.h ${MAKESTRERR}
	@echo generating ${.TARGET}
	@${AWK} -f ${MAKESTRERR} ${.CURDIR}/dos_errno.h > ${.TARGET}\
		|| ( rm -f ${.TARGET}; exit 1 )

# only needed during build
libinstall::

.include <bsd.lib.mk>
