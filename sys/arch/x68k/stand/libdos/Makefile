#	$NetBSD: Makefile,v 1.12 2013/05/24 16:04:37 christos Exp $

LIB=	dos

NOLINT=# defined
NOPROFILE=# defined
NOPIC=# defined

.include "../Makefile.booters"

CPPFLAGS+=	-I${.CURDIR} -I.

SRCS=	dos_rename.S
SRCS+=	dos_cerror.S dos_procerr.S dos_errno.c dos_strerror.S
CLEANFILES+=	dos_strerror.S

ASM=	exit.S getchar.S putchar.S .Sminp.S .S.Sut.S pr.Sut.S in.Sut.S \
	inkey.S getc.S print.S gets.S keysns.S kflushgp.S kflush.S.S \
	kflushin.S kflushgc.S kflushgs.S fflush.S chgdrv.S drvctrl.S \
	.Snsns.S prnsns.S cinsns.S .Sutsns.S fatchk.S fatchk2.S hendsp.S.S \
	hendspmp.S hendspmr.S hendspmc.S hendsp.S.S hendspip.S hendspir.S \
	hendspic.S hendsp.S.S hendspsp.S hendspsr.S hendspsc.S curdrv.S \
	getss.S fgetc.S fgets.S fputc.S fputs.S allc.Sse.S super.S fnckeygt.S \
	fnckeyst.S c_putc.S c_print.S c_.S.Sr.S c_.Scate.S c_.Swn_s.S \
	c_up_s.S c_up.S c_.Swn.S c_right.S c_left.S c_cls_ed.S c_cls_st.S \
	c_cls_al.S c_era_ed.S c_era_st.S c_era_al.S c_ins.S c_del.S \
	c_fnk.Sd.S c_win.Sw.S c_width.S c_cu.Sn.S c_cu.Sff.S k_keyinp.S \
	k_keysns.S k_sftsns.S k_keybit.S k_ins.Sd.S intvcs.S pspset.S \
	gettim2.S settim2.S namests.S getdate.S setdate.S gettime.S settime.S \
	verify.S dup0.S vernum.S keeppr.S getdpb.S breakck.S drvxchg.S \
	intvcg.S dskfre.S nameck.S mkdir.S rmdir.S chdir.S create.S.Spen.S \
	c.Sse.S read.S write.S delete.S seek.S ch.Sd.S .Sctrlgt.S .Sctrlst.S \
	.Sctrlrh.S .Sctrlwh.S .Sctrlrd.S .Sctrlwd.S .Sctrlis.S .Sctr.Ss.S \
	.Sctrldvgt.S .Sctrlfdgt.S .Sctrlrtset.S .Sctrldvctl.S .Sctrlfdctl.S \
	dup.S dup2.S curdir.S mal.Sc.S mfree.S setb.Sck.S .Sadexec.S .Sad.S \
	pathchk.S .Sa.Snly.S exe.Snly.S bind.S.S exec2.S exit2.S wait.S \
	files.S exfiles.S nfiles.S exnfiles.S setpdb.S getpdb.S setenv.S \
	getenv.S verifyg.S .Sm.Sn_ck.S .Sm.Sn_rd.S .Sm.Sn_wt.S .Sm.Sn_lk.S \
	.Sm.Sn_fre.S .Sm.Sn_del.S .Sve.S filedate.S mal.Sc2.S mal.Sc0.S \
	maketmp.S newfile.S .Sck.S un.Sck.S getassign.S makeassign.S \
	rassign.S fflush_set.S.Ss_patch.S get_fcb_adr.S s_mal.Sc.S \
	s_mal.Sc0.S s_mfree.S s_p.Scess.S retshell.S ctla.Srt.S erra.Srt.S \
	diskred.S diskred2.S diskwrt.S diskwrt2.S in.Ssflg.S super_jsr.S \
	memcpy.S.Spen_pr.S kill_pr.S get_pr.S suspend_pr.S sleep_pr.S \
	send_pr.S time_pr.S change_pr.S bus_err.S open.S malloc.S close.S \
	fflush_set.S

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
