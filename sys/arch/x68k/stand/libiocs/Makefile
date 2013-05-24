#	$NetBSD: Makefile,v 1.11 2013/05/24 16:00:16 christos Exp $

LIB=	iocs

NOLINT=# defined
NOPROFILE=# defined
NOPIC=# defined

.include "../Makefile.booters"

CPPFLAGS+=	-I.

ASM=	b_keyinp.S b_keysns.S b_sftsns.S bitsns.S skeyset.S tvctrl.S \
	ledmod.S tgusemd.S defchr.S crtmod.S contrast.S hsvtorgb.S tpalet.S \
	tpalet2.S tcolor.S fntget.S textget.S textput.S clipput.S scroll.S \
	b_curon.S b_curoff.S b_putc.S b_print.S b_color.S b_locate.S \
	b_down_s.S b_up_s.S b_up.S b_down.S b_right.S b_left.S b_clr_ed.S \
	b_clr_st.S b_clr_al.S b_era_ed.S b_era_st.S b_era_al.S b_ins.S \
	b_del.S b_consol.S b_putmes.S set232c.S lof232c.S inp232c.S \
	isns232c.S osns232c.S out232c.S joyget.S init_prn.S snsprn.S \
	outlpt.S outprn.S b_seek.S b_verify.S b_readdi.S b_dskini.S \
	b_drvsns.S b_write.S b_read.S b_recali.S b_assign.S b_writed.S \
	b_readid.S b_badfmt.S b_readdl.S b_format.S b_drvchk.S b_eject.S \
	bindatebcd.S bindateset.S timebcd.S timeset.S bindateget.S datebin.S \
	timeget.S timebin.S datecnv.S timecnv.S dateasc.S timeasc.S dayasc.S \
	alarmmod.S alarmset.S alarmget.S adpcmout.S adpcminp.S adpcmaot.S \
	adpcmain.S adpcmlot.S adpcmlin.S adpcmsns.S adpcmmod.S opmset.S \
	opmsns.S opmintst.S timerdst.S vdispst.S crtcras.S hsyncst.S \
	prnintst.S ms_init.S ms_curon.S ms_curof.S ms_stat.S ms_getdt.S \
	ms_curgt.S ms_curst.S ms_limit.S ms_offtm.S ms_ontm.S ms_patst.S \
	ms_sel.S ms_sel2.S skey_mod.S densns.S ontime.S b_intvcs.S b_super.S \
	b_bpeek.S b_wpeek.S b_lpeek.S b_memstr.S b_bpoke.S b_wpoke.S \
	b_lpoke.S b_memset.S dmamove.S dmamov_a.S dmamov_l.S dmamode.S \
	bootinf.S romver.S g_clr_on.S gpalet.S sftjis.S jissft.S akconv.S \
	rmacnv.S dakjob.S hanjob.S mpu_stat.S cache_st.S cache_md.S \
	b_curmod.S b_curpat.S b_curpat1.S b_curdef.S b_scroll.S os_curon.S \
	os_curof.S drawmode.S apage.S vpage.S home.S window.S wipe.S pset.S \
	point.S line.S box.S fill.S circle.S paint.S symbol.S getgrm.S \
	putgrm.S sp_init.S sp_on.S sp_off.S sp_cgclr.S sp_defcg.S sp_gtpcg.S \
	sp_regst.S sp_reggt.S bgscrlst.S bgscrlgt.S bgctrlst.S bgctrlgt.S \
	bgtextcl.S bgtextst.S bgtextgt.S spalet.S txxline.S txyline.S \
	txline.S txbox.S txfill.S txrev.S txrascpy.S abortrst.S iplerr.S \
	abortjob.S trap15.S

# SCSI calls
ASM+=	s_reset.S s_select.S s_cmdout.S s_datain.S s_dataout.S s_stsin.S \
	s_msgin.S s_msgout.S s_phase.S s_inquiry.S s_read.S s_write.S \
	s_format.S s_testunit.S s_readcap.S s_readext.S s_writeext.S \
	s_rezerounit.S s_request.S s_seek.S s_startstop.S s_reassign.S \
	s_pamedium.S

SRCS+=	${ASM}

MAKECALL=${.CURDIR}/makeiocscalls.awk

${ASM}: ${.CURDIR}/iocs.h ${MAKECALL}
	${_MKTARGET_CREATE}
	@grep -i 'IOCS_${.PREFIX} ' ${.CURDIR}/iocs.h | \
	    ${TOOL_AWK} -f ${MAKECALL} | \
	    ${CPP} ${CPPFLAGS} > ${.TARGET}

CLEANFILES+=	${ASM}

# only needed during build
libinstall::

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>
