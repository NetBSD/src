#	$NetBSD: Makefile,v 1.5 2019/08/27 12:45:04 msaitoh Exp $

NOMAN=	# defined

.include <bsd.own.mk>

.if ${MKRADEONFIRMWARE} != "no"

FILES=	\
	R100_cp.bin \
	R200_cp.bin \
	R300_cp.bin \
	R420_cp.bin \
	R520_cp.bin \
	RS600_cp.bin \
	RS690_cp.bin \
	R600_me.bin R600_pfp.bin R600_rlc.bin R600_uvd.bin \
	RS780_me.bin RS780_pfp.bin RS780_uvd.bin \
	RV610_me.bin RV610_pfp.bin \
	RV620_me.bin RV620_pfp.bin \
	RV630_me.bin RV630_pfp.bin \
	RV635_me.bin RV635_pfp.bin \
	RV670_me.bin RV670_pfp.bin \
	RV710_me.bin RV710_pfp.bin RV710_smc.bin RV710_uvd.bin \
	RV730_me.bin RV730_pfp.bin RV730_smc.bin \
	RV740_smc.bin \
	RV770_me.bin RV770_pfp.bin RV770_smc.bin RV770_uvd.bin \
	R700_rlc.bin \
	ARUBA_me.bin ARUBA_pfp.bin ARUBA_rlc.bin \
	BARTS_mc.bin BARTS_me.bin BARTS_pfp.bin BARTS_smc.bin \
	BONAIRE_ce.bin BONAIRE_mc.bin BONAIRE_mc2.bin BONAIRE_me.bin \
	BONAIRE_mec.bin BONAIRE_pfp.bin BONAIRE_rlc.bin BONAIRE_sdma.bin \
	BONAIRE_smc.bin BONAIRE_uvd.bin BONAIRE_vce.bin \
	BTC_rlc.bin \
	CAICOS_mc.bin CAICOS_me.bin CAICOS_pfp.bin CAICOS_smc.bin \
	CAYMAN_mc.bin CAYMAN_me.bin CAYMAN_pfp.bin CAYMAN_rlc.bin \
	CAYMAN_smc.bin \
	CEDAR_me.bin CEDAR_pfp.bin CEDAR_rlc.bin CEDAR_smc.bin \
	CYPRESS_me.bin CYPRESS_pfp.bin CYPRESS_rlc.bin CYPRESS_smc.bin \
	CYPRESS_uvd.bin \
	HAINAN_ce.bin HAINAN_mc.bin HAINAN_mc2.bin HAINAN_me.bin \
	HAINAN_pfp.bin HAINAN_rlc.bin HAINAN_smc.bin \
	HAWAII_ce.bin HAWAII_mc.bin HAWAII_mc2.bin HAWAII_me.bin \
	HAWAII_mec.bin HAWAII_pfp.bin HAWAII_rlc.bin HAWAII_sdma.bin \
	HAWAII_smc.bin \
	JUNIPER_me.bin JUNIPER_pfp.bin JUNIPER_rlc.bin JUNIPER_smc.bin \
	KABINI_ce.bin KABINI_me.bin KABINI_mec.bin KABINI_pfp.bin \
	KABINI_rlc.bin KABINI_sdma.bin \
	KAVERI_ce.bin KAVERI_me.bin KAVERI_mec.bin KAVERI_pfp.bin \
	KAVERI_rlc.bin KAVERI_sdma.bin \
	MULLINS_ce.bin MULLINS_me.bin MULLINS_mec.bin MULLINS_pfp.bin \
	MULLINS_rlc.bin MULLINS_sdma.bin \
	OLAND_ce.bin OLAND_mc.bin OLAND_mc2.bin OLAND_me.bin OLAND_pfp.bin \
	OLAND_rlc.bin OLAND_smc.bin \
	PALM_me.bin PALM_pfp.bin \
	PITCAIRN_ce.bin PITCAIRN_mc.bin PITCAIRN_mc2.bin PITCAIRN_me.bin \
	PITCAIRN_pfp.bin PITCAIRN_rlc.bin PITCAIRN_smc.bin \
	REDWOOD_me.bin REDWOOD_pfp.bin REDWOOD_rlc.bin REDWOOD_smc.bin \
	SUMO_me.bin SUMO_pfp.bin SUMO_rlc.bin SUMO_uvd.bin \
	SUMO2_me.bin SUMO2_pfp.bin \
	TAHITI_ce.bin TAHITI_mc.bin TAHITI_mc2.bin TAHITI_me.bin \
	TAHITI_pfp.bin TAHITI_rlc.bin TAHITI_smc.bin TAHITI_uvd.bin \
	TAHITI_vce.bin \
	TURKS_mc.bin TURKS_me.bin TURKS_pfp.bin TURKS_smc.bin \
	VERDE_ce.bin VERDE_mc.bin VERDE_mc2.bin VERDE_me.bin VERDE_pfp.bin \
	VERDE_rlc.bin VERDE_smc.bin \
	bonaire_ce.bin bonaire_mc.bin bonaire_me.bin bonaire_mec.bin \
	bonaire_pfp.bin bonaire_rlc.bin bonaire_sdma.bin bonaire_smc.bin \
	bonaire_uvd.bin \
	hainan_ce.bin hainan_mc.bin hainan_me.bin hainan_pfp.bin \
	hainan_rlc.bin hainan_smc.bin \
	hawaii_ce.bin hawaii_mc.bin hawaii_me.bin hawaii_mec.bin \
	hawaii_pfp.bin hawaii_rlc.bin hawaii_sdma.bin hawaii_smc.bin \
	kabini_ce.bin kabini_me.bin kabini_mec.bin kabini_pfp.bin \
	kabini_rlc.bin kabini_sdma.bin \
	kaveri_ce.bin kaveri_me.bin kaveri_mec.bin kaveri_mec2.bin \
	kaveri_pfp.bin kaveri_rlc.bin kaveri_sdma.bin \
	mullins_ce.bin mullins_me.bin mullins_mec.bin mullins_pfp.bin \
	mullins_rlc.bin mullins_sdma.bin \
	oland_ce.bin oland_mc.bin oland_me.bin oland_pfp.bin oland_rlc.bin \
	oland_smc.bin \
	pitcairn_ce.bin pitcairn_mc.bin pitcairn_me.bin pitcairn_pfp.bin \
	pitcairn_rlc.bin pitcairn_smc.bin \
	tahiti_ce.bin tahiti_mc.bin tahiti_me.bin tahiti_pfp.bin \
	tahiti_rlc.bin tahiti_smc.bin verde_ce.bin verde_mc.bin verde_me.bin \
	verde_pfp.bin verde_rlc.bin verde_smc.bin

# radeondrmks needs these after mountroot, but before starting init.
FIRMWAREDIR=	/libdata/firmware

.endif

# XXX
includes:

clean:

cleandir:

.include <bsd.files.mk>
