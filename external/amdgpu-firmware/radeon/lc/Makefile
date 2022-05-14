#	$NetBSD: Makefile,v 1.1 2022/05/14 20:54:31 mrg Exp $

NOMAN=	# defined

.include <bsd.own.mk>

FILES=	\
	banks_k_2_smc.bin \
	bonaire_ce.bin bonaire_k_smc.bin bonaire_mc.bin bonaire_me.bin \
	bonaire_mec.bin bonaire_pfp.bin bonaire_rlc.bin bonaire_sdma.bin \
	bonaire_sdma1.bin bonaire_smc.bin bonaire_uvd.bin bonaire_vce.bin \
	hainan_ce.bin hainan_k_smc.bin hainan_mc.bin hainan_me.bin \
	hainan_pfp.bin hainan_rlc.bin hainan_smc.bin hawaii_ce.bin \
	hawaii_k_smc.bin hawaii_mc.bin hawaii_me.bin hawaii_mec.bin \
	hawaii_pfp.bin hawaii_rlc.bin hawaii_sdma.bin hawaii_sdma1.bin \
	hawaii_smc.bin hawaii_uvd.bin hawaii_vce.bin \
	kabini_ce.bin kabini_me.bin kabini_mec.bin kabini_pfp.bin \
	kabini_rlc.bin kabini_sdma.bin kabini_sdma1.bin kabini_uvd.bin \
	kabini_vce.bin \
	kaveri_ce.bin kaveri_me.bin kaveri_mec.bin kaveri_mec2.bin \
	kaveri_pfp.bin kaveri_rlc.bin kaveri_sdma.bin kaveri_sdma1.bin \
	kaveri_uvd.bin kaveri_vce.bin \
	mullins_ce.bin mullins_me.bin mullins_mec.bin mullins_pfp.bin \
	mullins_rlc.bin mullins_sdma.bin mullins_sdma1.bin mullins_uvd.bin \
	mullins_vce.bin \
	oland_ce.bin oland_k_smc.bin oland_mc.bin oland_me.bin \
	oland_pfp.bin oland_rlc.bin oland_smc.bin \
	pitcairn_ce.bin pitcairn_k_smc.bin pitcairn_mc.bin pitcairn_me.bin \
	pitcairn_pfp.bin pitcairn_rlc.bin pitcairn_smc.bin \
	si58_mc.bin \
	tahiti_ce.bin tahiti_k_smc.bin tahiti_mc.bin tahiti_me.bin \
	tahiti_pfp.bin tahiti_rlc.bin tahiti_smc.bin \
	verde_ce.bin verde_k_smc.bin verde_mc.bin verde_me.bin \
	verde_pfp.bin verde_rlc.bin verde_smc.bin

# radeondrmkms needs these after mountroot, but before starting init.
# ... but see the comment in ../README why we put these lower case ones
# in /usr.
FILESDIR=	/usr/libdata/firmware/radeon

.PATH: ${.CURDIR}/../dist/lc

# XXX
includes:

clean:

cleandir:

.include <bsd.files.mk>
