# $NetBSD: Makefile,v 1.2 2018/02/04 03:19:52 christos Exp $
.include "../Makefile.inc"

CPPFLAGS+=-DAPPID=\"TSPI\" -DVAR_PREFIX=\"/var\" -DETC_PREFIX=\"/etc\"
CPPFLAGS+=-DTSS_BUILD_TSS12 -DTSS_BUILD_TRANSPORT -DTSS_BUILD_TICK
CPPFLAGS+=-DTSS_BUILD_COUNTER -DTSS_BUILD_AUTH -DTSS_BUILD_GETSET
CPPFLAGS+=-DTSS_BUILD_RANDOM -DTSS_BUILD_CAPS -DTSS_BUILD_CAPS_TPM
CPPFLAGS+=-DTSS_BUILD_POLICY -DTSS_BUILD_DIR -DTSS_BUILD_PCR_EVENTS
CPPFLAGS+=-DTSS_BUILD_HASH -DTSS_BUILD_SIGN -DTSS_BUILD_QUOTE
CPPFLAGS+=-DTSS_BUILD_PCR_COMP -DTSS_BUILD_SEAL -DTSS_BUILD_CHANGEAUTH
CPPFLAGS+=-DTSS_BUILD_BIND -DTSS_BUILD_OWN -DTSS_BUILD_PS -DTSS_BUILD_ADMIN
CPPFLAGS+=-DTSS_BUILD_AIK -DTSS_BUILD_EK  -DTSS_BUILD_CERTIFY -DTSS_BUILD_KEY
CPPFLAGS+=-DTSS_BUILD_MAINT  -DTSS_BUILD_MIGRATION -DTSS_BUILD_PCR_EXTEND
CPPFLAGS+=-DTSS_BUILD_SELFTEST -DTSS_BUILD_PCRS_LIST -DTSS_BUILD_HASH_LIST
CPPFLAGS+=-DTSS_BUILD_ENCDATA_LIST -DTSS_BUILD_RSAKEY_LIST -DTSS_BUILD_ASN1
CPPFLAGS+=-DTSS_BUILD_AUDIT -DTSS_BUILD_SEALX -DTSS_BUILD_QUOTE2
CPPFLAGS+=-DTSS_BUILD_NV -DTSS_BUILD_DELEGATION -DTSS_BUILD_CMK

TSPI=${DIST}/src/tspi 
TRSPI=${DIST}/src/trspi 
.PATH.c: ${TSPI} ${TSPI}/rpc ${TSPI}/rpc/tcstp ${TSPI}/ps ${TRSPI} ${TRSPI}/crypto/${EXTERNAL_OPENSSL_SUBDIR}

SHLIB_MAJOR=3
SHLIB_MINOR=0

LIBDPLIBS+=	crypto	${NETBSDSRCDIR}/crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto
LIB=tspi
SRCS=\
hosttable.c \
log.c \
obj.c \
obj_context.c \
obj_delfamily.c \
obj_encdata.c \
obj_hash.c \
obj_migdata.c \
obj_nv.c \
obj_pcrs.c \
obj_policy.c \
obj_rsakey.c \
obj_tpm.c \
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
spi_utils.c \
ssl_ui.c \
tcs_api.c \
tsp_admin.c \
tsp_aik.c \
tsp_asym.c \
tsp_audit.c \
tsp_auth.c \
tsp_bind.c \
tsp_caps.c \
tsp_caps_tpm.c \
tsp_certify.c \
tsp_changeauth.c \
tsp_context_mem.c \
tsp_counter.c \
tsp_delegate.c \
tsp_dir.c \
tsp_ek.c \
tsp_get_flags.c \
tsp_key.c \
tsp_maint.c \
tsp_migration.c \
tsp_nv.c \
tsp_oper.c \
tsp_own.c \
tsp_pcr.c \
tsp_pcr_extend.c \
tsp_policy.c \
tsp_ps.c \
tsp_quote.c \
tsp_quote2.c \
tsp_random.c \
tsp_seal.c \
tsp_selftest.c \
tsp_sign.c \
tsp_tick.c \
tspi_admin.c \
tspi_aik.c \
tspi_asn1.c \
tspi_audit.c \
tspi_bind.c \
tspi_caps.c \
tspi_caps_tpm.c \
tspi_certify.c \
tspi_changeauth.c \
tspi_cmk.c \
tspi_context.c \
tspi_counter.c \
tspi_delegate.c \
tspi_dir.c \
tspi_ek.c \
tspi_getset.c \
tspi_hash.c \
tspi_key.c \
tspi_maint.c \
tspi_migration.c \
tspi_nv.c \
tspi_oper.c \
tspi_own.c \
tspi_pcr_comp.c \
tspi_pcr_comp12.c \
tspi_pcr_events.c \
tspi_pcr_extend.c \
tspi_policy.c \
tspi_ps.c \
tspi_quote.c \
tspi_quote2.c \
tspi_random.c \
tspi_seal.c \
tspi_selftest.c \
tspi_sign.c \
tspi_tick.c \
tspi_transport.c \
tspps.c \
hash.c \
rsa.c \
symmetric.c \
trousers.c 

.SUFFIXES: .h
.PATH.h: ${DIST}/src/include
INCSDIR=/usr/include
INCS=\
trousers/trousers.h \
trousers/tss.h \
tss/compat11b.h \
tss/platform.h \
tss/tcpa_defines.h \
tss/tcpa_error.h \
tss/tcpa_struct.h \
tss/tcpa_typedef.h \
tss/tcs.h \
tss/tcs_defines.h \
tss/tcs_error.h \
tss/tcs_structs.h \
tss/tcs_typedef.h \
tss/tddl_error.h \
tss/tddlapi_error.h \
tss/tddli.h \
tss/tpm.h \
tss/tpm_error.h \
tss/tpm_ordinal.h \
tss/tspi.h \
tss/tss_defines.h \
tss/tss_error.h \
tss/tss_error_basics.h \
tss/tss_structs.h \
tss/tss_typedef.h

.SUFFIXES: .3
.PATH.3: ${DIST}/man/man3
MAN=\
Tspi_ChangeAuth.3 \
Tspi_ChangeAuthAsym.3 \
Tspi_Context_Close.3 \
Tspi_Context_CloseObject.3 \
Tspi_Context_Connect.3 \
Tspi_Context_Create.3 \
Tspi_Context_CreateObject.3 \
Tspi_Context_FreeMemory.3 \
Tspi_Context_GetCapability.3 \
Tspi_Context_GetDefaultPolicy.3 \
Tspi_Context_GetKeyByPublicInfo.3 \
Tspi_Context_GetKeyByUUID.3 \
Tspi_Context_GetRegisteredKeysByUUID.3 \
Tspi_Context_GetRegisteredKeysByUUID2.3 \
Tspi_Context_GetTpmObject.3 \
Tspi_Context_LoadKeyByBlob.3 \
Tspi_Context_LoadKeyByUUID.3 \
Tspi_Context_RegisterKey.3 \
Tspi_Context_UnregisterKey.3 \
Tspi_DAA_IssueCredential.3 \
Tspi_DAA_IssueInit.3 \
Tspi_DAA_IssueSetup.3 \
Tspi_DAA_IssuerKeyVerification.3 \
Tspi_DAA_VerifyInit.3 \
Tspi_DAA_VerifySignature.3 \
Tspi_Data_Bind.3 \
Tspi_Data_Seal.3 \
Tspi_Data_Unbind.3 \
Tspi_Data_Unseal.3 \
Tspi_DecodeBER_TssBlob.3 \
Tspi_EncodeDER_TssBlob.3 \
Tspi_GetAttribData.3 \
Tspi_GetAttribUint32.3 \
Tspi_GetPolicyObject.3 \
Tspi_Hash_GetHashValue.3 \
Tspi_Hash_SetHashValue.3 \
Tspi_Hash_Sign.3 \
Tspi_Hash_UpdateHashValue.3 \
Tspi_Hash_VerifySignature.3 \
Tspi_Key_CertifyKey.3 \
Tspi_Key_ConvertMigrationBlob.3 \
Tspi_Key_CreateKey.3 \
Tspi_Key_CreateMigrationBlob.3 \
Tspi_Key_GetPubKey.3 \
Tspi_Key_LoadKey.3 \
Tspi_Key_UnloadKey.3 \
Tspi_Key_WrapKey.3 \
Tspi_PcrComposite_GetPcrValue.3 \
Tspi_PcrComposite_SelectPcrIndex.3 \
Tspi_PcrComposite_SetPcrValue.3 \
Tspi_Policy_AssignToObject.3 \
Tspi_Policy_FlushSecret.3 \
Tspi_Policy_SetSecret.3 \
Tspi_SetAttribData.3 \
Tspi_SetAttribUint32.3 \
Tspi_TPM_AuthorizeMigrationTicket.3 \
Tspi_TPM_CMKSetRestrictions.3 \
Tspi_TPM_CertifySelfTest.3 \
Tspi_TPM_CheckMaintenancePubKey.3 \
Tspi_TPM_ClearOwner.3 \
Tspi_TPM_CollateIdentityRequest.3 \
Tspi_TPM_CreateEndorsementKey.3 \
Tspi_TPM_CreateMaintenanceArchive.3 \
Tspi_TPM_DAA_JoinCreateDaaPubKey.3 \
Tspi_TPM_DAA_JoinInit.3 \
Tspi_TPM_DAA_JoinStoreCredential.3 \
Tspi_TPM_DAA_Sign.3 \
Tspi_TPM_DirRead.3 \
Tspi_TPM_DirWrite.3 \
Tspi_TPM_GetAuditDigest.3 \
Tspi_TPM_GetCapability.3 \
Tspi_TPM_GetEvent.3 \
Tspi_TPM_GetEventLog.3 \
Tspi_TPM_GetEvents.3 \
Tspi_TPM_GetPubEndorsementKey.3 \
Tspi_TPM_GetRandom.3 \
Tspi_TPM_GetStatus.3 \
Tspi_TPM_GetTestResult.3 \
Tspi_TPM_KillMaintenanceFeature.3 \
Tspi_TPM_LoadMaintenancePubKey.3 \
Tspi_TPM_OwnerGetSRKPubKey.3 \
Tspi_TPM_PcrExtend.3 \
Tspi_TPM_PcrRead.3 \
Tspi_TPM_Quote.3 \
Tspi_TPM_SelfTestFull.3 \
Tspi_TPM_SetStatus.3 \
Tspi_TPM_StirRandom.3 \
Tspi_TPM_TakeOwnership.3

.include <bsd.lib.mk>
