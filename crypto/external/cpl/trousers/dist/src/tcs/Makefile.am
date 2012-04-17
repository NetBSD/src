noinst_LIBRARIES=libtcs.a

CFLAGS+=-I${top_srcdir}/src/include
libtcs_a_LIBADD=${top_builddir}/src/tddl/libtddl.a
libtcs_a_CFLAGS=-DAPPID=\"TCSD\ TCS\" -DVAR_PREFIX=\"@localstatedir@\" -DETC_PREFIX=\"@sysconfdir@\"

libtcs_a_SOURCES=log.c \
		 tcs_caps.c \
		 tcs_req_mgr.c \
		 tcs_context.c \
		 tcsi_context.c \
		 tcs_utils.c \
		 rpc/@RPC@/rpc.c rpc/@RPC@/rpc_context.c \
		 tcsi_caps_tpm.c rpc/@RPC@/rpc_caps_tpm.c \
		 tcs_auth_mgr.c tcsi_auth.c rpc/@RPC@/rpc_auth.c \
		 tcs_pbg.c

if TSS_BUILD_TRANSPORT
libtcs_a_SOURCES+=tcsi_transport.c rpc/@RPC@/rpc_transport.c
libtcs_a_CFLAGS+=-DTSS_BUILD_TRANSPORT
endif
if TSS_BUILD_TICK
libtcs_a_SOURCES+=tcsi_tick.c rpc/@RPC@/rpc_tick.c
libtcs_a_CFLAGS+=-DTSS_BUILD_TICK
endif
if TSS_BUILD_COUNTER
libtcs_a_SOURCES+=tcsi_counter.c tcs_counter.c rpc/@RPC@/rpc_counter.c
libtcs_a_CFLAGS+=-DTSS_BUILD_COUNTER
endif
if TSS_BUILD_RANDOM
libtcs_a_SOURCES+=tcsi_random.c rpc/@RPC@/rpc_random.c
libtcs_a_CFLAGS+=-DTSS_BUILD_RANDOM
endif
if TSS_BUILD_CAPS
libtcs_a_SOURCES+=tcsi_caps.c rpc/@RPC@/rpc_caps.c
libtcs_a_CFLAGS+=-DTSS_BUILD_CAPS
endif
if TSS_BUILD_DIR
libtcs_a_SOURCES+=tcsi_dir.c rpc/@RPC@/rpc_dir.c
libtcs_a_CFLAGS+=-DTSS_BUILD_DIR
endif
if TSS_BUILD_PCR_EVENTS
libtcs_a_SOURCES+=tcsi_evlog.c tcs_evlog_biosem.c tcs_evlog_imaem.c tcs_evlog.c \
		  rpc/@RPC@/rpc_evlog.c
libtcs_a_CFLAGS+=-DTSS_BUILD_PCR_EVENTS
endif
if TSS_BUILD_SIGN
libtcs_a_SOURCES+=tcsi_sign.c rpc/@RPC@/rpc_sign.c
libtcs_a_CFLAGS+=-DTSS_BUILD_SIGN
endif
if TSS_BUILD_QUOTE
libtcs_a_SOURCES+=tcsi_quote.c tcs_quote.c rpc/@RPC@/rpc_quote.c
libtcs_a_CFLAGS+=-DTSS_BUILD_QUOTE
endif
if TSS_BUILD_SEAL
libtcs_a_SOURCES+=tcsi_seal.c tcs_seal.c rpc/@RPC@/rpc_seal.c
libtcs_a_CFLAGS+=-DTSS_BUILD_SEAL
endif
if TSS_BUILD_CHANGEAUTH
libtcs_a_SOURCES+=tcsi_changeauth.c rpc/@RPC@/rpc_changeauth.c
libtcs_a_CFLAGS+=-DTSS_BUILD_CHANGEAUTH
endif
if TSS_BUILD_BIND
libtcs_a_SOURCES+=tcsi_bind.c rpc/@RPC@/rpc_bind.c
libtcs_a_CFLAGS+=-DTSS_BUILD_BIND
endif
if TSS_BUILD_OWN
libtcs_a_SOURCES+=tcsi_own.c rpc/@RPC@/rpc_own.c
libtcs_a_CFLAGS+=-DTSS_BUILD_OWN
endif
if TSS_BUILD_PS
libtcs_a_SOURCES+=ps/ps_utils.c ps/tcsps.c tcsi_ps.c tcs_ps.c tcs_key_ps.c rpc/@RPC@/rpc_ps.c
libtcs_a_CFLAGS+=-DTSS_BUILD_PS
endif
if TSS_BUILD_ADMIN
libtcs_a_SOURCES+=tcsi_admin.c rpc/@RPC@/rpc_admin.c
libtcs_a_CFLAGS+=-DTSS_BUILD_ADMIN
endif
if TSS_BUILD_AIK
libtcs_a_SOURCES+=tcsi_aik.c tcs_aik.c rpc/@RPC@/rpc_aik.c
libtcs_a_CFLAGS+=-DTSS_BUILD_AIK
endif
if TSS_BUILD_EK
libtcs_a_SOURCES+=tcsi_ek.c rpc/@RPC@/rpc_ek.c
libtcs_a_CFLAGS+=-DTSS_BUILD_EK
endif
if TSS_BUILD_CERTIFY
libtcs_a_SOURCES+=tcsi_certify.c rpc/@RPC@/rpc_certify.c
libtcs_a_CFLAGS+=-DTSS_BUILD_CERTIFY
endif
if TSS_BUILD_KEY
libtcs_a_SOURCES+=tcsi_key.c tcs_key.c tcs_key_mem_cache.c tcs_context_key.c rpc/@RPC@/rpc_key.c \
		  crypto/@CRYPTO_PACKAGE@/crypto.c
libtcs_a_CFLAGS+=-DTSS_BUILD_KEY
endif
if TSS_BUILD_MAINT
libtcs_a_SOURCES+=tcsi_maint.c rpc/@RPC@/rpc_maint.c
libtcs_a_CFLAGS+=-DTSS_BUILD_MAINT
endif
if TSS_BUILD_MIGRATION
libtcs_a_SOURCES+=tcsi_migration.c tcs_migration.c rpc/@RPC@/rpc_migration.c
libtcs_a_CFLAGS+=-DTSS_BUILD_MIGRATION
endif
if TSS_BUILD_PCR_EXTEND
libtcs_a_SOURCES+=tcsi_pcr.c rpc/@RPC@/rpc_pcr_extend.c
libtcs_a_CFLAGS+=-DTSS_BUILD_PCR_EXTEND
endif
if TSS_BUILD_SELFTEST
libtcs_a_SOURCES+=tcsi_selftest.c rpc/@RPC@/rpc_selftest.c
libtcs_a_CFLAGS+=-DTSS_BUILD_SELFTEST
endif
if TSS_BUILD_DAA
libtcs_a_SOURCES+=tcsi_daa.c rpc/@RPC@/rpc_daa.c
libtcs_a_CFLAGS+=-DTSS_BUILD_DAA
endif
if TSS_BUILD_NV
libtcs_a_SOURCES+=tcsi_nv.c rpc/@RPC@/rpc_nv.c
libtcs_a_CFLAGS+=-DTSS_BUILD_NV
endif
if TSS_BUILD_AUDIT
libtcs_a_SOURCES+=tcsi_audit.c rpc/@RPC@/rpc_audit.c
libtcs_a_CFLAGS+=-DTSS_BUILD_AUDIT
endif
if TSS_BUILD_SEALX
libtcs_a_CFLAGS+=-DTSS_BUILD_SEALX
endif
if TSS_BUILD_TSS12
libtcs_a_SOURCES+=tcsi_oper.c rpc/@RPC@/rpc_oper.c
libtcs_a_CFLAGS+=-DTSS_BUILD_TSS12
endif
if TSS_BUILD_DELEGATION
libtcs_a_SOURCES+=tcsi_delegate.c rpc/@RPC@/rpc_delegate.c
libtcs_a_CFLAGS+=-DTSS_BUILD_DELEGATION
endif
if TSS_BUILD_QUOTE2
libtcs_a_SOURCES+=tcsi_quote2.c tcs_quote2.c rpc/@RPC@/rpc_quote2.c
libtcs_a_CFLAGS+=-DTSS_BUILD_QUOTE2
endif
if TSS_BUILD_CMK
libtcs_a_SOURCES+=tcsi_cmk.c rpc/@RPC@/rpc_cmk.c
libtcs_a_CFLAGS+=-DTSS_BUILD_CMK
endif
