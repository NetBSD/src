lib_LTLIBRARIES=libtspi.la

libtspi_la_LIBADD=${top_builddir}/src/trspi/libtrousers.la

# On setting -version-info, from the libtool manual:
#
# -version-info current:revision:age
#
# 1. Start with version information of 0:0:0 for each libtool library.
# 2. Update the version information only immediately before a public release of your software.
#    More frequent updates are unnecessary, and only guarantee that the current interface
#    number gets larger faster.
# 3. If the library source code has changed at all since the last update, then increment
#    revision (c:r:a becomes c:r+1:a).
# 4. If any interfaces have been added, removed, or changed since the last update, increment
#    current, and set revision to 0.
# 5. If any interfaces have been added since the last public release, then increment age.
# 6. If any interfaces have been removed since the last public release, then set age to 0.

libtspi_la_LDFLAGS=-version-info 3:0:2 -lpthread @CRYPTOLIB@

libtspi_la_CFLAGS=-I$(top_srcdir)/src/include -DAPPID=\"TSPI\" -DVAR_PREFIX=\"@localstatedir@\" -DETC_PREFIX=\"@sysconfdir@\"

libtspi_la_SOURCES=log.c \
                   spi_utils.c \
                   obj.c \
                   obj_policy.c \
                   tsp_policy.c \
                   obj_tpm.c \
                   obj_context.c \
                   tsp_context_mem.c \
                   tspi_context.c \
                   rpc/@RPC@/rpc_context.c \
                   rpc/tcs_api.c \
                   rpc/hosttable.c \
                   rpc/@RPC@/rpc.c

if TSS_BUILD_ASYM_CRYPTO
libtspi_la_SOURCES+=tsp_asym.c
endif
if TSS_BUILD_TSS12
# This is for individual APIs that exist outside TSS 1.2, but may have some TSS 1.2 internal
# features/options such as Tspi_TPM_SetStatus
libtspi_la_SOURCES+=tspi_oper.c tsp_oper.c rpc/@RPC@/rpc_oper.c
libtspi_la_CFLAGS+=-DTSS_BUILD_TSS12
endif
if TSS_BUILD_TRANSPORT
libtspi_la_SOURCES+=tspi_transport.c rpc/@RPC@/rpc_transport.c
libtspi_la_CFLAGS+=-DTSS_BUILD_TRANSPORT
endif
if TSS_BUILD_TICK
libtspi_la_SOURCES+=tspi_tick.c tsp_tick.c rpc/@RPC@/rpc_tick.c
libtspi_la_CFLAGS+=-DTSS_BUILD_TICK
endif
if TSS_BUILD_COUNTER
libtspi_la_SOURCES+=tspi_counter.c tsp_counter.c rpc/@RPC@/rpc_counter.c
libtspi_la_CFLAGS+=-DTSS_BUILD_COUNTER
endif
if TSS_BUILD_PCR_COMP12
libtspi_la_SOURCES+=tspi_pcr_comp12.c
endif
if TSS_BUILD_AUTH
libtspi_la_SOURCES+=tsp_auth.c rpc/@RPC@/rpc_auth.c
libtspi_la_CFLAGS+=-DTSS_BUILD_AUTH
endif
if TSS_BUILD_GETSET
libtspi_la_SOURCES+=tspi_getset.c
libtspi_la_CFLAGS+=-DTSS_BUILD_GETSET
endif
if TSS_BUILD_RANDOM
libtspi_la_SOURCES+=tspi_random.c tsp_random.c rpc/@RPC@/rpc_random.c
libtspi_la_CFLAGS+=-DTSS_BUILD_RANDOM
endif
if TSS_BUILD_CAPS
libtspi_la_SOURCES+=tspi_caps.c tsp_caps.c rpc/@RPC@/rpc_caps.c
libtspi_la_CFLAGS+=-DTSS_BUILD_CAPS
endif
if TSS_BUILD_CAPS_TPM
libtspi_la_SOURCES+=tspi_caps_tpm.c tsp_caps_tpm.c rpc/@RPC@/rpc_caps_tpm.c
libtspi_la_CFLAGS+=-DTSS_BUILD_CAPS_TPM
endif
if TSS_BUILD_POLICY
libtspi_la_SOURCES+=tspi_policy.c
libtspi_la_CFLAGS+=-DTSS_BUILD_POLICY
endif
if TSS_BUILD_DIR
libtspi_la_SOURCES+=tspi_dir.c tsp_dir.c rpc/@RPC@/rpc_dir.c
libtspi_la_CFLAGS+=-DTSS_BUILD_DIR
endif
if TSS_BUILD_PCR_EVENTS
libtspi_la_SOURCES+=tspi_pcr_events.c rpc/@RPC@/rpc_evlog.c
libtspi_la_CFLAGS+=-DTSS_BUILD_PCR_EVENTS
endif
if TSS_BUILD_HASH
libtspi_la_SOURCES+=tspi_hash.c
libtspi_la_CFLAGS+=-DTSS_BUILD_HASH
endif
if TSS_BUILD_SIGN
libtspi_la_SOURCES+=tspi_sign.c tsp_sign.c rpc/@RPC@/rpc_sign.c
libtspi_la_CFLAGS+=-DTSS_BUILD_SIGN
endif
if TSS_BUILD_QUOTE
libtspi_la_SOURCES+=tspi_quote.c tsp_quote.c rpc/@RPC@/rpc_quote.c
libtspi_la_CFLAGS+=-DTSS_BUILD_QUOTE
endif
if TSS_BUILD_PCR_COMP
libtspi_la_SOURCES+=tspi_pcr_comp.c
libtspi_la_CFLAGS+=-DTSS_BUILD_PCR_COMP
endif
if TSS_BUILD_SEAL
libtspi_la_SOURCES+=tspi_seal.c rpc/@RPC@/rpc_seal.c
libtspi_la_CFLAGS+=-DTSS_BUILD_SEAL
endif
if TSS_BUILD_CHANGEAUTH
libtspi_la_SOURCES+=tspi_changeauth.c tsp_changeauth.c rpc/@RPC@/rpc_changeauth.c
libtspi_la_CFLAGS+=-DTSS_BUILD_CHANGEAUTH
endif
if TSS_BUILD_BIND
libtspi_la_SOURCES+=tspi_bind.c tsp_bind.c rpc/@RPC@/rpc_bind.c
libtspi_la_CFLAGS+=-DTSS_BUILD_BIND
endif
if TSS_BUILD_OWN
libtspi_la_SOURCES+=tsp_own.c tspi_own.c rpc/@RPC@/rpc_own.c
libtspi_la_CFLAGS+=-DTSS_BUILD_OWN
endif
if TSS_BUILD_PS
libtspi_la_SOURCES+=ps/ps_utils.c ps/tspps.c tspi_ps.c rpc/@RPC@/rpc_ps.c tsp_ps.c
libtspi_la_CFLAGS+=-DTSS_BUILD_PS
endif
if TSS_BUILD_ADMIN
libtspi_la_SOURCES+=tspi_admin.c tsp_admin.c rpc/@RPC@/rpc_admin.c
libtspi_la_CFLAGS+=-DTSS_BUILD_ADMIN
endif
if TSS_BUILD_AIK
libtspi_la_SOURCES+=tspi_aik.c tsp_aik.c rpc/@RPC@/rpc_aik.c
libtspi_la_CFLAGS+=-DTSS_BUILD_AIK
endif
if TSS_BUILD_EK
libtspi_la_SOURCES+=tspi_ek.c tsp_ek.c rpc/@RPC@/rpc_ek.c
libtspi_la_CFLAGS+=-DTSS_BUILD_EK
endif
if TSS_BUILD_CERTIFY
libtspi_la_SOURCES+=tspi_certify.c tsp_certify.c rpc/@RPC@/rpc_certify.c
libtspi_la_CFLAGS+=-DTSS_BUILD_CERTIFY
endif
if TSS_BUILD_KEY
libtspi_la_SOURCES+=tspi_key.c rpc/@RPC@/rpc_key.c
libtspi_la_CFLAGS+=-DTSS_BUILD_KEY
endif
if TSS_BUILD_MAINT
libtspi_la_SOURCES+=tspi_maint.c tsp_maint.c rpc/@RPC@/rpc_maint.c
libtspi_la_CFLAGS+=-DTSS_BUILD_MAINT
endif
if TSS_BUILD_MIGRATION
libtspi_la_SOURCES+=tspi_migration.c tsp_migration.c rpc/@RPC@/rpc_migration.c
libtspi_la_CFLAGS+=-DTSS_BUILD_MIGRATION
endif
if TSS_BUILD_PCR_EXTEND
libtspi_la_SOURCES+=tspi_pcr_extend.c tsp_pcr_extend.c rpc/@RPC@/rpc_pcr_extend.c
libtspi_la_CFLAGS+=-DTSS_BUILD_PCR_EXTEND
endif
if TSS_BUILD_SELFTEST
libtspi_la_SOURCES+=tspi_selftest.c tsp_selftest.c rpc/@RPC@/rpc_selftest.c
libtspi_la_CFLAGS+=-DTSS_BUILD_SELFTEST
endif
if TSS_BUILD_DAA
libtspi_la_SOURCES+=tspi_daa.c tsp_daa.c \
		    rpc/@RPC@/rpc_daa.c \
		    daa/daa_issuer/keypair_generator.c daa/daa_issuer/prime_gen.c \
		    daa/daa_issuer/key_correctness_proof.c daa/daa_platform/platform.c \
		    daa/daa_issuer/issuer_init.c daa/daa_issuer/issue_credential.c \
		    daa/daa_verifier/verifier_transaction.c daa/daa_verifier/verifier.c \
		    daa/daa_structs.c daa/daa_parameter.c daa/big_integer/bi_gmp.c \
		    daa/big_integer/bi_openssl.c daa/daa_anonymityrevocation/csencryption_result.c \
		    daa/big_integer/bi.c daa/utils/list.c
libtspi_la_CFLAGS+=-DTSS_BUILD_DAA
endif
if TSS_BUILD_GET_FLAGS
libtspi_la_SOURCES+=tsp_get_flags.c
endif
if TSS_BUILD_PCRS_LIST
libtspi_la_SOURCES+=obj_pcrs.c tsp_pcr.c
libtspi_la_CFLAGS+=-DTSS_BUILD_PCRS_LIST
endif
if TSS_BUILD_HASH_LIST
libtspi_la_SOURCES+=obj_hash.c
libtspi_la_CFLAGS+=-DTSS_BUILD_HASH_LIST
endif
if TSS_BUILD_ENCDATA_LIST
libtspi_la_SOURCES+=obj_encdata.c
libtspi_la_CFLAGS+=-DTSS_BUILD_ENCDATA_LIST
endif
if TSS_BUILD_RSAKEY_LIST
libtspi_la_SOURCES+=obj_rsakey.c tsp_key.c
libtspi_la_CFLAGS+=-DTSS_BUILD_RSAKEY_LIST
endif
if TSS_BUILD_ASN1
libtspi_la_SOURCES+=tspi_asn1.c
libtspi_la_CFLAGS+=-DTSS_BUILD_ASN1
endif
if TSS_BUILD_AUDIT
libtspi_la_SOURCES+=tspi_audit.c tsp_audit.c rpc/@RPC@/rpc_audit.c
libtspi_la_CFLAGS+=-DTSS_BUILD_AUDIT
endif
if TSS_BUILD_SEALX
libtspi_la_SOURCES+=tsp_seal.c
libtspi_la_CFLAGS+=-DTSS_BUILD_SEALX
endif
if TSS_BUILD_QUOTE2
libtspi_la_SOURCES+=tspi_quote2.c tsp_quote2.c rpc/@RPC@/rpc_quote2.c
libtspi_la_CFLAGS+=-DTSS_BUILD_QUOTE2
endif

if HAVE_GTK
libtspi_la_CFLAGS+=@GTK_CFLAGS@
libtspi_la_LDFLAGS+=@GTK_LIBS@
libtspi_la_SOURCES+=gtk/main.c gtk/support.c gtk/interface.c gtk/callbacks.c
endif
if OPENSSL_UI
libtspi_la_LDFLAGS+=-lssl
libtspi_la_SOURCES+=ssl_ui.c
endif

if TSS_BUILD_NV
libtspi_la_SOURCES+=tspi_nv.c obj_nv.c tsp_nv.c rpc/@RPC@/rpc_nv.c
libtspi_la_CFLAGS+=-DTSS_BUILD_NV
endif
if TSS_BUILD_DELEGATION
libtspi_la_SOURCES+=tspi_delegate.c tsp_delegate.c obj_delfamily.c rpc/@RPC@/rpc_delegate.c
libtspi_la_CFLAGS+=-DTSS_BUILD_DELEGATION
endif
if TSS_BUILD_CMK
libtspi_la_SOURCES+=tspi_cmk.c obj_migdata.c rpc/@RPC@/rpc_cmk.c
libtspi_la_CFLAGS+=-DTSS_BUILD_CMK
endif
