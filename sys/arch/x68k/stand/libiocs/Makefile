#	$NetBSD: Makefile,v 1.2 1999/02/13 02:54:50 lukem Exp $

LIB=	iocs

MKLINT=no
MKPROFILE=no
MKPIC=no

AWK?=	awk

ASM=	b_keyinp.o b_keysns.o b_sftsns.o bitsns.o skeyset.o tvctrl.o \
	ledmod.o tgusemd.o defchr.o crtmod.o contrast.o hsvtorgb.o tpalet.o \
	tpalet2.o tcolor.o fntget.o textget.o textput.o clipput.o scroll.o \
	b_curon.o b_curoff.o b_putc.o b_print.o b_color.o b_locate.o \
	b_down_s.o b_up_s.o b_up.o b_down.o b_right.o b_left.o b_clr_ed.o \
	b_clr_st.o b_clr_al.o b_era_ed.o b_era_st.o b_era_al.o b_ins.o \
	b_del.o b_consol.o b_putmes.o set232c.o lof232c.o inp232c.o \
	isns232c.o osns232c.o out232c.o joyget.o init_prn.o snsprn.o \
	outlpt.o outprn.o b_seek.o b_verify.o b_readdi.o b_dskini.o \
	b_drvsns.o b_write.o b_read.o b_recali.o b_assign.o b_writed.o \
	b_readid.o b_badfmt.o b_readdl.o b_format.o b_drvchk.o b_eject.o \
	bindatebcd.o bindateset.o timebcd.o timeset.o bindateget.o datebin.o \
	timeget.o timebin.o datecnv.o timecnv.o dateasc.o timeasc.o dayasc.o \
	alarmmod.o alarmset.o alarmget.o adpcmout.o adpcminp.o adpcmaot.o \
	adpcmain.o adpcmlot.o adpcmlin.o adpcmsns.o adpcmmod.o opmset.o \
	opmsns.o opmintst.o timerdst.o vdispst.o crtcras.o hsyncst.o \
	prnintst.o ms_init.o ms_curon.o ms_curof.o ms_stat.o ms_getdt.o \
	ms_curgt.o ms_curst.o ms_limit.o ms_offtm.o ms_ontm.o ms_patst.o \
	ms_sel.o ms_sel2.o skey_mod.o densns.o ontime.o b_intvcs.o b_super.o \
	b_bpeek.o b_wpeek.o b_lpeek.o b_memstr.o b_bpoke.o b_wpoke.o \
	b_lpoke.o b_memset.o dmamove.o dmamov_a.o dmamov_l.o dmamode.o \
	bootinf.o romver.o g_clr_on.o gpalet.o sftjis.o jissft.o akconv.o \
	rmacnv.o dakjob.o hanjob.o mpu_stat.o cache_st.o cache_md.o \
	b_curmod.o b_curpat.o b_curpat1.o b_curdef.o b_scroll.o os_curon.o \
	os_curof.o drawmode.o apage.o vpage.o home.o window.o wipe.o pset.o \
	point.o line.o box.o fill.o circle.o paint.o symbol.o getgrm.o \
	putgrm.o sp_init.o sp_on.o sp_off.o sp_cgclr.o sp_defcg.o sp_gtpcg.o \
	sp_regst.o sp_reggt.o bgscrlst.o bgscrlgt.o bgctrlst.o bgctrlgt.o \
	bgtextcl.o bgtextst.o bgtextgt.o spalet.o txxline.o txyline.o \
	txline.o txbox.o txfill.o txrev.o txrascpy.o abortrst.o iplerr.o \
	abortjob.o trap15.o

# SCSI calls
ASM+=	s_reset.o s_select.o s_cmdout.o s_datain.o s_dataout.o s_stsin.o \
	s_msgin.o s_msgout.o s_phase.o s_inquiry.o s_read.o s_write.o \
	s_format.o s_testunit.o s_readcap.o s_readext.o s_writeext.o \
	s_rezerounit.o s_request.o s_seek.o s_startstop.o s_reassign.o \
	s_pamedium.o

OBJS+=	${ASM}

MAKECALL=${.CURDIR}/makeiocscalls.awk

${ASM}: ${.CURDIR}/iocs.h ${MAKECALL}
	@echo creating ${.TARGET}
	@grep -i 'IOCS_${.PREFIX} ' ${.CURDIR}/iocs.h | \
	    ${AWK} -f ${MAKECALL} | ${AS} -o ${.TARGET}

# only needed during build
libinstall::

.include <bsd.lib.mk>
