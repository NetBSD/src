#	$NetBSD: Makefile,v 1.13 2013/05/26 05:52:13 isaki Exp $

LIB=	dos

NOLINT=# defined
NOPROFILE=# defined
NOPIC=# defined

.include "../Makefile.booters"

CPPFLAGS+=	-I${.CURDIR} -I.

SRCS=	dos_rename.S
SRCS+=	dos_cerror.S dos_procerr.S dos_errno.c dos_strerror.S
CLEANFILES+=	dos_strerror.S

ASM=	exit.S getchar.S putchar.S cominp.S comout.S prnout.S inpout.S \
	inkey.S getc.S print.S gets.S keysns.S kflushgp.S kflushio.S \
	kflushin.S kflushgc.S kflushgs.S fflush.S chgdrv.S drvctrl.S \
	consns.S prnsns.S cinsns.S coutsns.S fatchk.S fatchk2.S hendspmo.S \
	hendspmp.S hendspmr.S hendspmc.S hendspio.S hendspip.S hendspir.S \
	hendspic.S hendspso.S hendspsp.S hendspsr.S hendspsc.S curdrv.S \
	getss.S fgetc.S fgets.S fputc.S fputs.S allclose.S super.S fnckeygt.S \
	fnckeyst.S c_putc.S c_print.S c_color.S c_locate.S c_down_s.S \
	c_up_s.S c_up.S c_down.S c_right.S c_left.S c_cls_ed.S c_cls_st.S \
	c_cls_al.S c_era_ed.S c_era_st.S c_era_al.S c_ins.S c_del.S \
	c_fnkmod.S c_window.S c_width.S c_curon.S c_curoff.S k_keyinp.S \
	k_keysns.S k_sftsns.S k_keybit.S k_insmod.S intvcs.S pspset.S \
	gettim2.S settim2.S namests.S getdate.S setdate.S gettime.S settime.S \
	verify.S dup0.S vernum.S keeppr.S getdpb.S breakck.S drvxchg.S \
	intvcg.S dskfre.S nameck.S mkdir.S rmdir.S chdir.S create.S open.S \
	close.S read.S write.S delete.S seek.S chmod.S ioctrlgt.S ioctrlst.S \
	ioctrlrh.S ioctrlwh.S ioctrlrd.S ioctrlwd.S ioctrlis.S ioctrlos.S \
	ioctrldvgt.S ioctrlfdgt.S ioctrlrtset.S ioctrldvctl.S ioctrlfdctl.S \
	dup.S dup2.S curdir.S malloc.S mfree.S setblock.S loadexec.S load.S \
	pathchk.S loadonly.S execonly.S bindno.S exec2.S exit2.S wait.S \
	files.S exfiles.S nfiles.S exnfiles.S setpdb.S getpdb.S setenv.S \
	getenv.S verifyg.S common_ck.S common_rd.S common_wt.S common_lk.S \
	common_fre.S common_del.S move.S filedate.S malloc2.S malloc0.S \
	maketmp.S newfile.S lock.S unlock.S getassign.S makeassign.S \
	rassign.S fflush_set.S os_patch.S get_fcb_adr.S s_malloc.S \
	s_malloc0.S s_mfree.S s_process.S retshell.S ctlabort.S errabort.S \
	diskred.S diskred2.S diskwrt.S diskwrt2.S indosflg.S super_jsr.S \
	memcpy.S open_pr.S kill_pr.S get_pr.S suspend_pr.S sleep_pr.S \
	send_pr.S time_pr.S change_pr.S

SRCS+=	${ASM}

MAKECALL=${.CURDIR}/makedoscalls.awk

${ASM}: ${.CURDIR}/dos.h ${MAKECALL}
	${_MKTARGET_CREATE}
	@grep -i 'DOS_${.PREFIX} ' ${.CURDIR}/dos.h | \
	    ${TOOL_AWK} -f ${MAKECALL} | ${CPP} ${CPPFLAGS} > ${.TARGET}

MAKESTRERR=${.CURDIR}/makestrerror.awk

CLEANFILES+=${ASM}

dos_strerror.S: ${.CURDIR}/dos_errno.h ${MAKESTRERR}
	${_MKTARGET_CREATE}
	@${TOOL_AWK} -f ${MAKESTRERR} ${.CURDIR}/dos_errno.h > ${.TARGET}\
		|| ( rm -f ${.TARGET}; exit 1 )

# only needed during build
libinstall::

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>
