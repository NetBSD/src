# $NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:02:23 yamt Exp $

LIBISPRIVATE=yes
.include "../Makefile.inc"

TCS=${DIST}/src/tcs
.PATH.c: ${TCS} ${TCS}/ps ${TCS}/rpc ${TCS}/rpc/tcstp ${TCS}/crypto/openssl

CPPFLAGS+=-DAPPID=\"TCSD\ TCS\" -DVAR_PREFIX=\"/var\" -DETC_PREFIX=\"/etc\"
CPPFLAGS+=-DTSS_BUILD_TRANSPORT -DTSS_BUILD_TICK -DTSS_BUILD_COUNTER
CPPFLAGS+=-DTSS_BUILD_RANDOM -DTSS_BUILD_CAPS -DTSS_BUILD_DIR
CPPFLAGS+=-DTSS_BUILD_PCR_EVENTS -DTSS_BUILD_SIGN -DTSS_BUILD_QUOTE
CPPFLAGS+=-DTSS_BUILD_SEAL -DTSS_BUILD_CHANGEAUTH -DTSS_BUILD_BIND
CPPFLAGS+=-DTSS_BUILD_OWN -DTSS_BUILD_PS -DTSS_BUILD_ADMIN
CPPFLAGS+=-DTSS_BUILD_AIK -DTSS_BUILD_EK -DTSS_BUILD_CERTIFY
CPPFLAGS+=-DTSS_BUILD_KEY -DTSS_BUILD_MAINT -DTSS_BUILD_MIGRATION
CPPFLAGS+=-DTSS_BUILD_PCR_EXTEND -DTSS_BUILD_SELFTEST -DTSS_BUILD_NV
CPPFLAGS+=-DTSS_BUILD_AUDIT -DTSS_BUILD_SEALX -DTSS_BUILD_TSS12
CPPFLAGS+=-DTSS_BUILD_DELEGATION -DTSS_BUILD_QUOTE2  -DTSS_BUILD_CMK

LIB=tcs
SRCS=\
crypto.c \
log.c \
ps_utils.c \
rpc.c \
rpc_admin.c \
rpc_aik.c \
rpc_audit.c \
rpc_auth.c \
rpc_bind.c \
rpc_caps.c \
rpc_caps_tpm.c \
rpc_certify.c \
rpc_changeauth.c \
rpc_cmk.c \
rpc_context.c \
rpc_counter.c \
rpc_delegate.c \
rpc_dir.c \
rpc_ek.c \
rpc_evlog.c \
rpc_key.c \
rpc_maint.c \
rpc_migration.c \
rpc_nv.c \
rpc_oper.c \
rpc_own.c \
rpc_pcr_extend.c \
rpc_ps.c \
rpc_quote.c \
rpc_quote2.c \
rpc_random.c \
rpc_seal.c \
rpc_selftest.c \
rpc_sign.c \
rpc_tick.c \
rpc_transport.c \
tcs_aik.c \
tcs_auth_mgr.c \
tcs_caps.c \
tcs_context.c \
tcs_context_key.c \
tcs_counter.c \
tcs_evlog.c \
tcs_evlog_biosem.c \
tcs_evlog_imaem.c \
tcs_key.c \
tcs_key_mem_cache.c \
tcs_key_ps.c \
tcs_migration.c \
tcs_pbg.c \
tcs_ps.c \
tcs_quote.c \
tcs_quote2.c \
tcs_req_mgr.c \
tcs_seal.c \
tcs_utils.c \
tcsi_admin.c \
tcsi_aik.c \
tcsi_audit.c \
tcsi_auth.c \
tcsi_bind.c \
tcsi_caps.c \
tcsi_caps_tpm.c \
tcsi_certify.c \
tcsi_changeauth.c \
tcsi_cmk.c \
tcsi_context.c \
tcsi_counter.c \
tcsi_delegate.c \
tcsi_dir.c \
tcsi_ek.c \
tcsi_evlog.c \
tcsi_key.c \
tcsi_maint.c \
tcsi_migration.c \
tcsi_nv.c \
tcsi_oper.c \
tcsi_own.c \
tcsi_pcr.c \
tcsi_ps.c \
tcsi_quote.c \
tcsi_quote2.c \
tcsi_random.c \
tcsi_seal.c \
tcsi_selftest.c \
tcsi_sign.c \
tcsi_tick.c \
tcsi_transport.c \
tcsps.c

.include <bsd.lib.mk>
