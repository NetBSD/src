
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _TCS_UTILS_H_
#define _TCS_UTILS_H_

#include <assert.h>

#include "threads.h"
#include "tcs_context.h"
#include "tcs_tsp.h"
#include "trousers_types.h"

struct key_mem_cache
{
	TCPA_KEY_HANDLE tpm_handle;
	TCS_KEY_HANDLE tcs_handle;
	UINT16 flags;
	int ref_cnt;
	UINT32 time_stamp;
	TSS_UUID uuid;
	TSS_UUID p_uuid;
	TSS_KEY *blob;
	struct key_mem_cache *parent;
	struct key_mem_cache *next, *prev;
};

extern struct key_mem_cache *key_mem_cache_head;
MUTEX_DECLARE_EXTERN(mem_cache_lock);

struct tpm_properties
{
	UINT32 num_pcrs;
	UINT32 num_dirs;
	UINT32 num_keys;
	UINT32 num_auths;
	TSS_BOOL authctx_swap;
	TSS_BOOL keyctx_swap;
	TPM_VERSION version;
	BYTE manufacturer[16];
};

extern struct tpm_properties tpm_metrics;

#define TPM_VERSION_IS(maj, min) \
	((tpm_metrics.version.major == maj) && (tpm_metrics.version.minor == min))

#define TSS_UUID_IS_OWNEREVICT(uuid) \
	((!uuid->ulTimeLow) && (!uuid->usTimeMid) && (!uuid->usTimeHigh) && \
	 (!uuid->bClockSeqHigh) && (!uuid->bClockSeqLow) && (!uuid->rgbNode[0]) && \
	 (!uuid->rgbNode[1]) && (!uuid->rgbNode[2]) && (!uuid->rgbNode[3]) && \
	 (uuid->rgbNode[4] == 1))

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

TSS_RESULT get_tpm_metrics(struct tpm_properties *);

TSS_RESULT auth_mgr_init();
TSS_RESULT auth_mgr_final();
TSS_RESULT auth_mgr_check(TCS_CONTEXT_HANDLE, TPM_AUTHHANDLE *);
TSS_RESULT auth_mgr_release_auth_handle(TCS_AUTHHANDLE, TCS_CONTEXT_HANDLE, TSS_BOOL);
void       auth_mgr_release_auth(TPM_AUTH *, TPM_AUTH *, TCS_CONTEXT_HANDLE);
TSS_RESULT auth_mgr_oiap(TCS_CONTEXT_HANDLE, TCS_AUTHHANDLE *, TCPA_NONCE *);
TSS_RESULT auth_mgr_osap(TCS_CONTEXT_HANDLE, TCPA_ENTITY_TYPE, UINT32, TCPA_NONCE,
			 TCS_AUTHHANDLE *, TCPA_NONCE *, TCPA_NONCE *);
TSS_RESULT auth_mgr_close_context(TCS_CONTEXT_HANDLE);
TSS_RESULT auth_mgr_swap_out(TCS_CONTEXT_HANDLE);
TSS_BOOL   auth_mgr_req_new(TCS_CONTEXT_HANDLE);
TSS_RESULT auth_mgr_add(TCS_CONTEXT_HANDLE, TPM_AUTHHANDLE);

TSS_RESULT event_log_init();
TSS_RESULT event_log_final();
TSS_RESULT owner_evict_init();

#ifdef TSS_BUILD_PCR_EVENTS
#define EVENT_LOG_init()	event_log_init()
#define EVENT_LOG_final()	event_log_final()
#else
#define EVENT_LOG_init()	(TSS_SUCCESS)
#define EVENT_LOG_final()
#endif

#define TSS_TPM_TXBLOB_SIZE		(4096)
#define TSS_TXBLOB_WRAPPEDCMD_OFFSET	(TSS_TPM_TXBLOB_HDR_LEN + sizeof(UINT32))
#define TSS_MAX_AUTHS_CAP		(1024)
#define TSS_REQ_MGR_MAX_RETRIES		(5)

#define next( x ) x = x->next

TSS_RESULT key_mgr_dec_ref_count(TCS_KEY_HANDLE);
TSS_RESULT key_mgr_inc_ref_count(TCS_KEY_HANDLE);
void key_mgr_ref_count();
TSS_RESULT key_mgr_load_by_uuid(TCS_CONTEXT_HANDLE, TSS_UUID *, TCS_LOADKEY_INFO *,
				TCS_KEY_HANDLE *);
TSS_RESULT key_mgr_load_by_blob(TCS_CONTEXT_HANDLE, TCS_KEY_HANDLE, UINT32, BYTE *,
				TPM_AUTH *, TCS_KEY_HANDLE *, TCS_KEY_HANDLE *);
TSS_RESULT key_mgr_evict(TCS_CONTEXT_HANDLE, TCS_KEY_HANDLE);


extern TCS_CONTEXT_HANDLE InternalContext;

TSS_RESULT mc_update_time_stamp(TCPA_KEY_HANDLE);
TCS_KEY_HANDLE getNextTcsKeyHandle();
TCPA_STORE_PUBKEY *getParentPubBySlot(TCPA_KEY_HANDLE slot);
TCPA_STORE_PUBKEY *mc_get_pub_by_slot(TCPA_KEY_HANDLE);
TCPA_STORE_PUBKEY *mc_get_pub_by_handle(TCS_KEY_HANDLE);
TSS_UUID *mc_get_uuid_by_pub(TCPA_STORE_PUBKEY *);
TSS_RESULT mc_get_handles_by_uuid(TSS_UUID *, TCS_KEY_HANDLE *, TCPA_KEY_HANDLE *);
TCS_KEY_HANDLE mc_get_handle_by_encdata(BYTE *);
TSS_RESULT mc_update_encdata(BYTE *, BYTE *);
TSS_RESULT mc_find_next_ownerevict_uuid(TSS_UUID *);
TSS_RESULT mc_set_uuid(TCS_KEY_HANDLE, TSS_UUID *);

TSS_RESULT initDiskCache(void);
void replaceEncData_PS(TSS_UUID, BYTE *encData, BYTE *newEncData);

TSS_RESULT mc_add_entry(TCS_KEY_HANDLE, TCPA_KEY_HANDLE, TSS_KEY *);
TSS_RESULT mc_add_entry_init(TCS_KEY_HANDLE, TCPA_KEY_HANDLE, TSS_KEY *, TSS_UUID *);
TSS_RESULT mc_remove_entry(TCS_KEY_HANDLE);
TSS_RESULT mc_set_slot_by_slot(TCPA_KEY_HANDLE, TCPA_KEY_HANDLE);
TSS_RESULT mc_set_slot_by_handle(TCS_KEY_HANDLE, TCPA_KEY_HANDLE);
TCPA_KEY_HANDLE mc_get_slot_by_handle(TCS_KEY_HANDLE);
TCPA_KEY_HANDLE mc_get_slot_by_handle_lock(TCS_KEY_HANDLE);
TCPA_KEY_HANDLE mc_get_slot_by_pub(TCPA_STORE_PUBKEY *);
TCS_KEY_HANDLE mc_get_handle_by_pub(TCPA_STORE_PUBKEY *, TCS_KEY_HANDLE);
TCPA_STORE_PUBKEY *mc_get_parent_pub_by_pub(TCPA_STORE_PUBKEY *);
TSS_BOOL isKeyRegistered(TCPA_STORE_PUBKEY *);
TSS_RESULT mc_get_blob_by_pub(TCPA_STORE_PUBKEY *, TSS_KEY **);
TSS_RESULT evictFirstKey(TCS_KEY_HANDLE);
TSS_RESULT getParentUUIDByUUID(TSS_UUID *, TSS_UUID *);
TSS_RESULT getRegisteredKeyByUUID(TSS_UUID *, BYTE *, UINT16 *);
TSS_RESULT isPubRegistered(TCPA_STORE_PUBKEY *);
TSS_RESULT getRegisteredUuidByPub(TCPA_STORE_PUBKEY *, TSS_UUID **);
TSS_RESULT getRegisteredKeyByPub(TCPA_STORE_PUBKEY *, UINT32 *, BYTE **);
TSS_BOOL isKeyLoaded(TCPA_KEY_HANDLE);
TSS_RESULT LoadKeyShim(TCS_CONTEXT_HANDLE, TCPA_STORE_PUBKEY *, TSS_UUID *,TCPA_KEY_HANDLE *);
TSS_RESULT mc_set_parent_by_handle(TCS_KEY_HANDLE, TCS_KEY_HANDLE);
TSS_RESULT isUUIDRegistered(TSS_UUID *, TSS_BOOL *);
void destroy_key_refs(TSS_KEY *);

/* cxt.c */
TSS_RESULT context_close_auth(TCS_CONTEXT_HANDLE);
TSS_RESULT checkContextForAuth(TCS_CONTEXT_HANDLE, TCS_AUTHHANDLE);
TSS_RESULT addContextForAuth(TCS_CONTEXT_HANDLE, TCS_AUTHHANDLE);
TSS_RESULT ctx_verify_context(TCS_CONTEXT_HANDLE);
COND_VAR *ctx_get_cond_var(TCS_CONTEXT_HANDLE);
TSS_RESULT ctx_mark_key_loaded(TCS_CONTEXT_HANDLE, TCS_KEY_HANDLE);
TSS_RESULT ctx_remove_key_loaded(TCS_CONTEXT_HANDLE, TCS_KEY_HANDLE);
TSS_BOOL ctx_has_key_loaded(TCS_CONTEXT_HANDLE, TCS_KEY_HANDLE);
void       ctx_ref_count_keys(struct tcs_context *);
struct tcs_context *get_context(TCS_CONTEXT_HANDLE);
TSS_RESULT ctx_req_exclusive_transport(TCS_CONTEXT_HANDLE);
TSS_RESULT ctx_set_transport_enabled(TCS_CONTEXT_HANDLE, TPM_TRANSHANDLE);
TSS_RESULT ctx_set_transport_disabled(TCS_CONTEXT_HANDLE, TCS_HANDLE *);

#ifdef TSS_BUILD_KEY
#define CTX_ref_count_keys(c)	ctx_ref_count_keys(c)
#define KEY_MGR_ref_count()	key_mgr_ref_count()
TSS_RESULT ensureKeyIsLoaded(TCS_CONTEXT_HANDLE, TCS_KEY_HANDLE, TCPA_KEY_HANDLE *);
#else
#define CTX_ref_count_keys(c)
#define KEY_MGR_ref_count()
#define ensureKeyIsLoaded(...)	(1 /* XXX non-zero return will indicate failure */)
#endif


TCS_CONTEXT_HANDLE make_context();
void destroy_context(TCS_CONTEXT_HANDLE);

/* tcs_utils.c */
TSS_RESULT get_current_version(TPM_VERSION *);
void LogData(char *string, UINT32 data);
void LogResult(char *string, TSS_RESULT result);
TSS_RESULT canILoadThisKey(TCPA_KEY_PARMS *parms, TSS_BOOL *);
TSS_RESULT internal_EvictByKeySlot(TCPA_KEY_HANDLE slot);

TSS_RESULT clearKeysFromChip(TCS_CONTEXT_HANDLE hContext);
TSS_RESULT clearUnknownKeys(TCS_CONTEXT_HANDLE, UINT32 *);

void UINT64ToArray(UINT64, BYTE *);
void UINT32ToArray(UINT32, BYTE *);
void UINT16ToArray(UINT16, BYTE *);
UINT64 Decode_UINT64(BYTE *);
UINT32 Decode_UINT32(BYTE *);
UINT16 Decode_UINT16(BYTE *);
void LoadBlob_UINT64(UINT64 *, UINT64, BYTE *);
void LoadBlob_UINT32(UINT64 *, UINT32, BYTE *);
void LoadBlob_UINT16(UINT64 *, UINT16, BYTE *);
void UnloadBlob_UINT64(UINT64 *, UINT64 *, BYTE *);
void UnloadBlob_UINT32(UINT64 *, UINT32 *, BYTE *);
void UnloadBlob_UINT16(UINT64 *, UINT16 *, BYTE *);
void LoadBlob_BYTE(UINT64 *, BYTE, BYTE *);
void UnloadBlob_BYTE(UINT64 *, BYTE *, BYTE *);
void LoadBlob_BOOL(UINT64 *, TSS_BOOL, BYTE *);
void UnloadBlob_BOOL(UINT64 *, TSS_BOOL *, BYTE *);
void LoadBlob(UINT64 *, UINT32, BYTE *, BYTE *);
void UnloadBlob(UINT64 *, UINT32, BYTE *, BYTE *);
void LoadBlob_Header(UINT16, UINT32, UINT32, BYTE *);
#ifdef TSS_DEBUG
#define UnloadBlob_Header(b,u)	LogUnloadBlob_Header(b,u, __FILE__, __LINE__)
TSS_RESULT LogUnloadBlob_Header(BYTE *, UINT32 *, char *, int);
#else
TSS_RESULT UnloadBlob_Header(BYTE *, UINT32 *);
#endif
TSS_RESULT UnloadBlob_MIGRATIONKEYAUTH(UINT64 *, BYTE *, TCPA_MIGRATIONKEYAUTH *);
void LoadBlob_Auth(UINT64 *, BYTE *, TPM_AUTH *);
void UnloadBlob_Auth(UINT64 *, BYTE *, TPM_AUTH *);
void LoadBlob_KEY_PARMS(UINT64 *, BYTE *, TCPA_KEY_PARMS *);
TSS_RESULT UnloadBlob_KEY_PARMS(UINT64 *, BYTE *, TCPA_KEY_PARMS *);
TSS_RESULT UnloadBlob_STORE_PUBKEY(UINT64 *, BYTE *, TCPA_STORE_PUBKEY *);
void LoadBlob_STORE_PUBKEY(UINT64 *, BYTE *, TCPA_STORE_PUBKEY *);
void UnloadBlob_VERSION(UINT64 *, BYTE *, TPM_VERSION *);
void LoadBlob_VERSION(UINT64 *, BYTE *, TPM_VERSION *);
void UnloadBlob_TCPA_VERSION(UINT64 *, BYTE *, TCPA_VERSION *);
void LoadBlob_TCPA_VERSION(UINT64 *, BYTE *, TCPA_VERSION *);
TSS_RESULT UnloadBlob_TSS_KEY(UINT64 *, BYTE *, TSS_KEY *);
void LoadBlob_TSS_KEY(UINT64 *, BYTE *, TSS_KEY *);
void LoadBlob_PUBKEY(UINT64 *, BYTE *, TCPA_PUBKEY *);
TSS_RESULT UnloadBlob_PUBKEY(UINT64 *, BYTE *, TCPA_PUBKEY *);
void LoadBlob_SYMMETRIC_KEY(UINT64 *, BYTE *, TCPA_SYMMETRIC_KEY *);
TSS_RESULT UnloadBlob_SYMMETRIC_KEY(UINT64 *, BYTE *, TCPA_SYMMETRIC_KEY *);
TSS_RESULT UnloadBlob_PCR_SELECTION(UINT64 *, BYTE *, TCPA_PCR_SELECTION *);
void LoadBlob_PCR_SELECTION(UINT64 *, BYTE *, TCPA_PCR_SELECTION);
TSS_RESULT UnloadBlob_PCR_COMPOSITE(UINT64 *, BYTE *, TCPA_PCR_COMPOSITE *);
void LoadBlob_PCR_INFO(UINT64 *, BYTE *, TCPA_PCR_INFO *);
TSS_RESULT UnloadBlob_PCR_INFO(UINT64 *, BYTE *, TCPA_PCR_INFO *);
TSS_RESULT UnloadBlob_STORED_DATA(UINT64 *, BYTE *, TCPA_STORED_DATA *);
void LoadBlob_STORED_DATA(UINT64 *, BYTE *, TCPA_STORED_DATA *);
void LoadBlob_KEY_FLAGS(UINT64 *, BYTE *, TCPA_KEY_FLAGS *);
void UnloadBlob_KEY_FLAGS(UINT64 *, BYTE *, TCPA_KEY_FLAGS *);
TSS_RESULT UnloadBlob_CERTIFY_INFO(UINT64 *, BYTE *, TCPA_CERTIFY_INFO *);
TSS_RESULT UnloadBlob_KEY_HANDLE_LIST(UINT64 *, BYTE *, TCPA_KEY_HANDLE_LIST *);
void LoadBlob_UUID(UINT64 *, BYTE *, TSS_UUID);
void UnloadBlob_UUID(UINT64 *, BYTE *, TSS_UUID *);
void LoadBlob_COUNTER_VALUE(UINT64 *, BYTE *, TPM_COUNTER_VALUE *);
void UnloadBlob_COUNTER_VALUE(UINT64 *, BYTE *, TPM_COUNTER_VALUE *);
void LoadBlob_DIGEST(UINT64 *, BYTE *, TPM_DIGEST *);
void UnloadBlob_DIGEST(UINT64 *, BYTE *, TPM_DIGEST *);
void LoadBlob_NONCE(UINT64 *, BYTE *, TPM_NONCE *);
void UnloadBlob_NONCE(UINT64 *, BYTE *, TPM_NONCE *);
void LoadBlob_AUTHDATA(UINT64 *, BYTE *, TPM_AUTHDATA *);
void UnloadBlob_AUTHDATA(UINT64 *, BYTE *, TPM_AUTHDATA *);
#define LoadBlob_ENCAUTH(a, b, c)	LoadBlob_AUTHDATA(a, b, c)
#define UnloadBlob_ENCAUTH(a, b, c)	UnloadBlob_AUTHDATA(a, b, c)

void UnloadBlob_CURRENT_TICKS(UINT64 *, BYTE *, TPM_CURRENT_TICKS *);
TSS_RESULT UnloadBlob_PCR_INFO_SHORT(UINT64 *, BYTE *, TPM_PCR_INFO_SHORT *);

TSS_RESULT Hash(UINT32, UINT32, BYTE *, BYTE *);
void free_external_events(UINT32, TSS_PCR_EVENT *);

TSS_RESULT internal_TerminateHandle(TCS_AUTHHANDLE handle);
UINT32 get_pcr_event_size(TSS_PCR_EVENT *);
TSS_RESULT fill_key_info(struct key_disk_cache *, struct key_mem_cache *, TSS_KM_KEYINFO *);
TSS_RESULT fill_key_info2(struct key_disk_cache *, struct key_mem_cache *, TSS_KM_KEYINFO2 *);

char platform_get_runlevel();
TSS_RESULT tpm_rsp_parse(TPM_COMMAND_CODE, BYTE *, UINT32, ...);
TSS_RESULT tpm_rqu_build(TPM_COMMAND_CODE, UINT64 *, BYTE *, ...);
TSS_RESULT tpm_preload_check(TCS_CONTEXT_HANDLE, TPM_COMMAND_CODE ordinal, ...);
TSS_RESULT getKeyByCacheEntry(struct key_disk_cache *, BYTE *, UINT16 *);
TSS_RESULT add_cache_entry(TCS_CONTEXT_HANDLE, BYTE *, TCS_KEY_HANDLE, TPM_KEY_HANDLE, TCS_KEY_HANDLE *);
TSS_RESULT get_slot(TCS_CONTEXT_HANDLE, TCS_KEY_HANDLE, TPM_KEY_HANDLE *);
TSS_RESULT get_slot_lite(TCS_CONTEXT_HANDLE, TCS_KEY_HANDLE, TPM_KEY_HANDLE *);
TSS_RESULT load_key_init(TPM_COMMAND_CODE, TCS_CONTEXT_HANDLE, TCS_KEY_HANDLE, UINT32, BYTE*, TSS_BOOL, TPM_AUTH*, TSS_BOOL*, UINT64*, BYTE*, TCS_KEY_HANDLE*, TPM_KEY_HANDLE*);
TSS_RESULT load_key_final(TCS_CONTEXT_HANDLE, TCS_KEY_HANDLE, TCS_KEY_HANDLE *, BYTE *, TPM_KEY_HANDLE);
TSS_RESULT LoadKeyByBlob_Internal(UINT32,TCS_CONTEXT_HANDLE,TCS_KEY_HANDLE,UINT32,BYTE *,TPM_AUTH *,
				  TCS_KEY_HANDLE *,TCS_KEY_HANDLE *);
TSS_RESULT TSC_PhysicalPresence_Internal(UINT16 physPres);
TSS_RESULT TCSP_FlushSpecific_Common(UINT32, TPM_RESOURCE_TYPE);

	TSS_RESULT TCSP_GetRegisteredKeyByPublicInfo_Internal(TCS_CONTEXT_HANDLE tcsContext, TCPA_ALGORITHM_ID algID,	/* in */
							       UINT32 ulPublicInfoLength,	/* in */
							       BYTE * rgbPublicInfo,	/* in */
							       UINT32 * keySize, BYTE ** keyBlob);

	TSS_RESULT TCS_OpenContext_Internal(TCS_CONTEXT_HANDLE * hContext	/* out  */
	    );

	TSS_RESULT TCS_CloseContext_Internal(TCS_CONTEXT_HANDLE hContext	/* in */
	    );

	TSS_RESULT TCS_FreeMemory_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					    BYTE * pMemory	/* in */
	    );

	TSS_RESULT TCS_LogPcrEvent_Internal(TCS_CONTEXT_HANDLE hContext,	/* in    */
					     TSS_PCR_EVENT Event,	/* in  */
					     UINT32 * pNumber	/* out */
	    );

	TSS_RESULT TCS_GetPcrEvent_Internal(TCS_CONTEXT_HANDLE hContext,	/* in  */
					     UINT32 PcrIndex,	/* in */
					     UINT32 * pNumber,	/* in, out */
					     TSS_PCR_EVENT ** ppEvent	/* out */
	    );

	TSS_RESULT TCS_GetPcrEventsByPcr_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						   UINT32 PcrIndex,	/* in */
						   UINT32 FirstEvent,	/* in */
						   UINT32 * pEventCount,	/* in,out */
						   TSS_PCR_EVENT ** ppEvents	/* out */
	    );

	TSS_RESULT TCS_GetPcrEventLog_Internal(TCS_CONTEXT_HANDLE hContext,	/* in  */
						UINT32 * pEventCount,	/* out */
						TSS_PCR_EVENT ** ppEvents	/* out */
	    );

	TSS_RESULT TCS_RegisterKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					     TSS_UUID *WrappingKeyUUID,	/* in */
					     TSS_UUID *KeyUUID,	/* in  */
					     UINT32 cKeySize,	/* in */
					     BYTE * rgbKey,	/* in */
					     UINT32 cVendorData,	/* in */
					     BYTE * gbVendorData	/* in */
	    );

	TSS_RESULT TCS_UnregisterKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						TSS_UUID KeyUUID	/* in  */
	    );

	TSS_RESULT TCS_EnumRegisteredKeys_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						    TSS_UUID * pKeyUUID,	/* in    */
						    UINT32 * pcKeyHierarchySize,	/* out */
						    TSS_KM_KEYINFO ** ppKeyHierarchy	/* out */
	    );
	
	TSS_RESULT TCS_EnumRegisteredKeys_Internal2(TCS_CONTEXT_HANDLE hContext,	/* in */
							    TSS_UUID * pKeyUUID,	/* in    */
							    UINT32 * pcKeyHierarchySize,	/* out */
							    TSS_KM_KEYINFO2 ** ppKeyHierarchy	/* out */
		);

	TSS_RESULT TCS_GetRegisteredKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						  TSS_UUID *KeyUUID,	/* in */
						  TSS_KM_KEYINFO ** ppKeyInfo	/* out */
	    );

	TSS_RESULT TCS_GetRegisteredKeyBlob_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						      TSS_UUID *KeyUUID,	/* in */
						      UINT32 * pcKeySize,	/* out */
						      BYTE ** prgbKey	/* out */
	    );

	TSS_RESULT TCSP_LoadKeyByBlob_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						TCS_KEY_HANDLE hUnwrappingKey,	/* in */
						UINT32 cWrappedKeyBlobSize,	/* in */
						BYTE * rgbWrappedKeyBlob,	/* in */
						TPM_AUTH * pAuth,	/* in, out */
						TCS_KEY_HANDLE * phKeyTCSI,	/* out */
						TCS_KEY_HANDLE * phKeyHMAC	/* out */
	    );

	TSS_RESULT TCSP_LoadKey2ByBlob_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						TCS_KEY_HANDLE hUnwrappingKey,	/* in */
						UINT32 cWrappedKeyBlobSize,	/* in */
						BYTE * rgbWrappedKeyBlob,	/* in */
						TPM_AUTH * pAuth,	/* in, out */
						TCS_KEY_HANDLE * phKeyTCSI	/* out */
	    );

	TSS_RESULT TCSP_LoadKeyByUUID_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						TSS_UUID *KeyUUID,	/* in */
						TCS_LOADKEY_INFO * pLoadKeyInfo,	/* in, out */
						TCS_KEY_HANDLE * phKeyTCSI	/* out */
	    );

	TSS_RESULT TCSP_EvictKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					   TCS_KEY_HANDLE hKey	/* in */
	    );

	TSS_RESULT TCSP_CreateWrapKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						TCS_KEY_HANDLE hWrappingKey,	/* in */
						TCPA_ENCAUTH KeyUsageAuth,	/* in */
						TCPA_ENCAUTH KeyMigrationAuth,	/* in */
						UINT32 keyInfoSize,	/* in */
						BYTE * keyInfo,	/* in */
						UINT32 * keyDataSize,	/* out */
						BYTE ** keyData,	/* out */
						TPM_AUTH * pAuth	/* in, out */
	    );

	TSS_RESULT TCSP_GetPubKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					    TCS_KEY_HANDLE hKey,	/* in */
					    TPM_AUTH * pAuth,	/* in, out */
					    UINT32 * pcPubKeySize,	/* out */
					    BYTE ** prgbPubKey	/* out */
	    );
	TSS_RESULT TCSP_MakeIdentity_Internal(TCS_CONTEXT_HANDLE hContext,	/* in  */
					       TCPA_ENCAUTH identityAuth,	/* in */
					       TCPA_CHOSENID_HASH IDLabel_PrivCAHash,	/* in */
					       UINT32 idKeyInfoSize,	/*in */
					       BYTE * idKeyInfo,	/*in */
					       TPM_AUTH * pSrkAuth,	/* in, out */
					       TPM_AUTH * pOwnerAuth,	/* in, out */
					       UINT32 * idKeySize,	/* out */
					       BYTE ** idKey,	/* out */
					       UINT32 * pcIdentityBindingSize,	/* out */
					       BYTE ** prgbIdentityBinding,	/* out */
					       UINT32 * pcEndorsementCredentialSize,	/* out */
					       BYTE ** prgbEndorsementCredential,	/* out */
					       UINT32 * pcPlatformCredentialSize,	/* out */
					       BYTE ** prgbPlatformCredential,	/* out */
					       UINT32 * pcConformanceCredentialSize,	/* out */
					       BYTE ** prgbConformanceCredential	/* out */
	    );

	TSS_RESULT TCSP_MakeIdentity2_Internal(TCS_CONTEXT_HANDLE hContext,	/* in  */
					       TCPA_ENCAUTH identityAuth,	/* in */
					       TCPA_CHOSENID_HASH IDLabel_PrivCAHash,	/* in */
					       UINT32 idKeyInfoSize,	/*in */
					       BYTE * idKeyInfo,	/*in */
					       TPM_AUTH * pSrkAuth,	/* in, out */
					       TPM_AUTH * pOwnerAuth,	/* in, out */
					       UINT32 * idKeySize,	/* out */
					       BYTE ** idKey,	/* out */
					       UINT32 * pcIdentityBindingSize,	/* out */
					       BYTE ** prgbIdentityBinding	/* out */
	    );

	TSS_RESULT TCS_GetCredential_Internal(TCS_CONTEXT_HANDLE hContext,	/* in  */
					      UINT32 ulCredentialType,		/* in */
					      UINT32 ulCredentialAccessMode,	/* in */
					      UINT32 * pulCredentialSize,	/* out */
					      BYTE ** prgbCredentialData	/* out */
	    );

	TSS_RESULT TCSP_SetOwnerInstall_Internal(TCS_CONTEXT_HANDLE hContext,   /* in */
						 TSS_BOOL state        /* in  */
	    );

	TSS_RESULT TCSP_TakeOwnership_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						UINT16 protocolID,	/* in */
						UINT32 encOwnerAuthSize,	/* in  */
						BYTE * encOwnerAuth,	/* in */
						UINT32 encSrkAuthSize,	/* in */
						BYTE * encSrkAuth,	/* in */
						UINT32 srkInfoSize,	/*in */
						BYTE * srkInfo,	/*in */
						TPM_AUTH * ownerAuth,	/* in, out */
						UINT32 * srkKeySize,	/*out */
						BYTE ** srkKey	/*out */
	    );

	TSS_RESULT TCSP_OIAP_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				       TCS_AUTHHANDLE * authHandle,	/* out  */
				       TCPA_NONCE * nonce0	/* out */
	    );

	TSS_RESULT TCSP_OSAP_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				       TCPA_ENTITY_TYPE entityType,	/* in */
				       UINT32 entityValue,	/* in */
				       TCPA_NONCE nonceOddOSAP,	/* in */
				       TCS_AUTHHANDLE * authHandle,	/* out  */
				       TCPA_NONCE * nonceEven,	/* out */
				       TCPA_NONCE * nonceEvenOSAP	/* out */
	    );

	TSS_RESULT TCSP_ChangeAuth_Internal(TCS_CONTEXT_HANDLE contextHandle,	/* in */
					     TCS_KEY_HANDLE parentHandle,	/* in */
					     TCPA_PROTOCOL_ID protocolID,	/* in */
					     TCPA_ENCAUTH newAuth,	/* in */
					     TCPA_ENTITY_TYPE entityType,	/* in */
					     UINT32 encDataSize,	/* in */
					     BYTE * encData,	/* in */
					     TPM_AUTH * ownerAuth,	/* in, out */
					     TPM_AUTH * entityAuth,	/* in, out       */
					     UINT32 * outDataSize,	/* out */
					     BYTE ** outData	/* out */
	    );

	TSS_RESULT TCSP_ChangeAuthOwner_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						  TCPA_PROTOCOL_ID protocolID,	/* in */
						  TCPA_ENCAUTH newAuth,	/* in */
						  TCPA_ENTITY_TYPE entityType,	/* in */
						  TPM_AUTH * ownerAuth	/* in, out */
	    );

	TSS_RESULT TCSP_ChangeAuthAsymStart_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						      TCS_KEY_HANDLE idHandle,	/* in */
						      TCPA_NONCE antiReplay,	/* in */
						      UINT32 KeySizeIn,	/* in */
						      BYTE * KeyDataIn,	/* in */
						      TPM_AUTH * pAuth,	/* in, out */
						      UINT32 * KeySizeOut,	/* out */
						      BYTE ** KeyDataOut,	/* out */
						      UINT32 * CertifyInfoSize,	/* out */
						      BYTE ** CertifyInfo,	/* out */
						      UINT32 * sigSize,	/* out */
						      BYTE ** sig,	/* out */
						      TCS_KEY_HANDLE * ephHandle	/* out */
	    );

	TSS_RESULT TCSP_ChangeAuthAsymFinish_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						       TCS_KEY_HANDLE parentHandle,	/* in */
						       TCS_KEY_HANDLE ephHandle,	/* in */
						       TCPA_ENTITY_TYPE entityType,	/* in */
						       TCPA_HMAC newAuthLink,	/* in */
						       UINT32 newAuthSize,	/* in */
						       BYTE * encNewAuth,	/* in */
						       UINT32 encDataSizeIn,	/* in */
						       BYTE * encDataIn,	/* in */
						       TPM_AUTH * ownerAuth,	/* in, out */
						       UINT32 * encDataSizeOut,	/* out */
						       BYTE ** encDataOut,	/* out */
						       TCPA_NONCE * saltNonce,	/* out */
						       TCPA_DIGEST * changeProof	/* out */
	    );

	TSS_RESULT TCSP_TerminateHandle_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						  TCS_AUTHHANDLE handle	/* in */
	    );

	TSS_RESULT TCSP_ActivateTPMIdentity_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						      TCS_KEY_HANDLE idKey,	/* in */
						      UINT32 blobSize,	/* in */
						      BYTE * blob,	/* in */
						      TPM_AUTH * idKeyAuth,	/* in, out */
						      TPM_AUTH * ownerAuth,	/* in, out */
						      UINT32 * SymmetricKeySize,	/* out */
						      BYTE ** SymmetricKey	/* out */
	    );

	TSS_RESULT TCSP_Extend_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					 TCPA_PCRINDEX pcrNum,	/* in */
					 TCPA_DIGEST inDigest,	/* in */
					 TCPA_PCRVALUE * outDigest	/* out */
	    );

	TSS_RESULT TCSP_PcrRead_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					  TCPA_PCRINDEX pcrNum,	/* in */
					  TCPA_PCRVALUE * outDigest	/* out */
	    );

	TSS_RESULT TCSP_PcrReset_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					  UINT32 pcrDataSizeIn,	/* in */
					  BYTE * pcrData	/* in */
	    );

	TSS_RESULT TCSP_Quote_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					TCS_KEY_HANDLE keyHandle,	/* in */
					TCPA_NONCE antiReplay,	/* in */
					UINT32 pcrDataSizeIn,	/* in */
					BYTE * pcrDataIn,	/* in */
					TPM_AUTH * privAuth,	/* in, out */
					UINT32 * pcrDataSizeOut,	/* out */
					BYTE ** pcrDataOut,	/* out */
					UINT32 * sigSize,	/* out */
					BYTE ** sig	/* out */
	    );
	
	TSS_RESULT TCSP_Quote2_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						TCS_KEY_HANDLE keyHandle,	/* in */
						TCPA_NONCE antiReplay,	/* in */
						UINT32 pcrDataSizeIn,	/* in */
						BYTE * pcrDataIn,	/* in */
						TSS_BOOL addVersion, /* in */
						TPM_AUTH * privAuth,	/* in, out */
						UINT32 * pcrDataSizeOut,	/* out */
						BYTE ** pcrDataOut,	/* out */
						UINT32 * versionInfoSize, /* out */
						BYTE ** versionInfo, /* out */
						UINT32 * sigSize,	/* out */
						BYTE ** sig	/* out */
		    );

	TSS_RESULT TCSP_DirWriteAuth_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					       TCPA_DIRINDEX dirIndex,	/* in */
					       TCPA_DIRVALUE newContents,	/* in */
					       TPM_AUTH * ownerAuth	/* in, out */
	    );

	TSS_RESULT TCSP_DirRead_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					  TCPA_DIRINDEX dirIndex,	/* in */
					  TCPA_DIRVALUE * dirValue	/* out */
	    );

	/* Since only the ordinal differs between Seal and Sealx (from an API point of view),
	   use a common Seal function specifying the ordinal to be sent to the TPM. */
	TSS_RESULT TCSP_Seal_Internal(UINT32 sealOrdinal,		/* in */
				       TCS_CONTEXT_HANDLE hContext,	/* in */
				       TCS_KEY_HANDLE keyHandle,	/* in */
				       TCPA_ENCAUTH encAuth,	/* in */
				       UINT32 pcrInfoSize,	/* in */
				       BYTE * PcrInfo,	/* in */
				       UINT32 inDataSize,	/* in */
				       BYTE * inData,	/* in */
				       TPM_AUTH * pubAuth,	/* in, out */
				       UINT32 * SealedDataSize,	/* out */
				       BYTE ** SealedData	/* out */
	    );

	TSS_RESULT TCSP_Unseal_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					 TCS_KEY_HANDLE parentHandle,	/* in */
					 UINT32 SealedDataSize,	/* in */
					 BYTE * SealedData,	/* in */
					 TPM_AUTH * parentAuth,	/* in, out */
					 TPM_AUTH * dataAuth,	/* in, out */
					 UINT32 * DataSize,	/* out */
					 BYTE ** Data	/* out */
	    );

	TSS_RESULT TCSP_UnBind_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					 TCS_KEY_HANDLE keyHandle,	/* in */
					 UINT32 inDataSize,	/* in */
					 BYTE * inData,	/* in */
					 TPM_AUTH * privAuth,	/* in, out */
					 UINT32 * outDataSize,	/* out */
					 BYTE ** outData	/* out */
	    );
	TSS_RESULT TCSP_CreateMigrationBlob_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						      TCS_KEY_HANDLE parentHandle,	/* in */
						      TCPA_MIGRATE_SCHEME migrationType,	/* in */
						      UINT32 MigrationKeyAuthSize,	/* in */
						      BYTE * MigrationKeyAuth,	/* in */
						      UINT32 encDataSize,	/* in */
						      BYTE * encData,	/* in */
						      TPM_AUTH * parentAuth,	/* in, out */
						      TPM_AUTH * entityAuth,	/* in, out */
						      UINT32 * randomSize,	/* out */
						      BYTE ** random,	/* out */
						      UINT32 * outDataSize,	/* out */
						      BYTE ** outData	/* out */
	    );

	TSS_RESULT TCSP_ConvertMigrationBlob_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						       TCS_KEY_HANDLE parentHandle,	/* in */
						       UINT32 inDataSize,	/* in */
						       BYTE * inData,	/* in */
						       UINT32 randomSize,	/* in */
						       BYTE * random,	/* in */
						       TPM_AUTH * parentAuth,	/* in, out */
						       UINT32 * outDataSize,	/* out */
						       BYTE ** outData	/* out */
	    );

	TSS_RESULT TCSP_AuthorizeMigrationKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
							TCPA_MIGRATE_SCHEME migrateScheme,	/* in */
							UINT32 MigrationKeySize,	/* in */
							BYTE * MigrationKey,	/* in */
							TPM_AUTH * ownerAuth,	/* in, out */
							UINT32 * MigrationKeyAuthSize,	/* out */
							BYTE ** MigrationKeyAuth	/* out */
	    );

	TSS_RESULT TCSP_CertifyKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					     TCS_KEY_HANDLE certHandle,	/* in */
					     TCS_KEY_HANDLE keyHandle,	/* in */
					     TCPA_NONCE antiReplay,	/* in */
					     TPM_AUTH * certAuth,	/* in, out */
					     TPM_AUTH * keyAuth,	/* in, out */
					     UINT32 * CertifyInfoSize,	/* out */
					     BYTE ** CertifyInfo,	/* out */
					     UINT32 * outDataSize,	/* out */
					     BYTE ** outData	/* out */
	    );

	TSS_RESULT TCSP_Sign_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				       TCS_KEY_HANDLE keyHandle,	/* in */
				       UINT32 areaToSignSize,	/* in */
				       BYTE * areaToSign,	/* in */
				       TPM_AUTH * privAuth,	/* in, out */
				       UINT32 * sigSize,	/* out */
				       BYTE ** sig	/* out */
	    );

	TSS_RESULT TCSP_GetRandom_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					    UINT32 * bytesRequested,	/* in, out */
					    BYTE ** randomBytes	/* out */
	    );

	TSS_RESULT TCSP_StirRandom_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					     UINT32 inDataSize,	/* in */
					     BYTE * inData	/* in */
	    );

	TSS_RESULT TCS_GetCapability_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					       TCPA_CAPABILITY_AREA capArea,	/* in */
					       UINT32 subCapSize,	/* in */
					       BYTE * subCap,	/* in */
					       UINT32 * respSize,	/* out */
					       BYTE ** resp	/* out */
	    );

	TSS_RESULT TCSP_GetCapability_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						TCPA_CAPABILITY_AREA capArea,	/* in */
						UINT32 subCapSize,	/* in */
						BYTE * subCap,	/* in */
						UINT32 * respSize,	/* out */
						BYTE ** resp	/* out */
	    );
	TSS_RESULT TCSP_SetCapability_Internal(TCS_CONTEXT_HANDLE hContext,        /* in */
					       TCPA_CAPABILITY_AREA capArea,       /* in */
					       UINT32 subCapSize,  /* in */
					       BYTE * subCap,      /* in */
					       UINT32 valueSize,   /* in */
					       BYTE * value,       /* in */
					       TPM_AUTH * pOwnerAuth      /* in, out */
	    );
	TSS_RESULT TCSP_GetCapabilityOwner_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						     TPM_AUTH * pOwnerAuth,	/* out */
						     TCPA_VERSION * pVersion,	/* out */
						     UINT32 * pNonVolatileFlags,	/* out */
						     UINT32 * pVolatileFlags	/* out */
	    );

	TSS_RESULT TCSP_CreateEndorsementKeyPair_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
							   TCPA_NONCE antiReplay,	/* in */
							   UINT32 endorsementKeyInfoSize,	/* in */
							   BYTE * endorsementKeyInfo,	/* in */
							   UINT32 * endorsementKeySize,	/* out */
							   BYTE ** endorsementKey,	/* out */
							   TCPA_DIGEST * checksum	/* out */
	    );

	TSS_RESULT TCSP_ReadPubek_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					    TCPA_NONCE antiReplay,	/* in */
					    UINT32 * pubEndorsementKeySize,	/* out */
					    BYTE ** pubEndorsementKey,	/* out */
					    TCPA_DIGEST * checksum	/* out */
	    );

	TSS_RESULT TCSP_DisablePubekRead_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						   TPM_AUTH * ownerAuth	/* in, out */
	    );

	TSS_RESULT TCSP_OwnerReadPubek_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						 TPM_AUTH * ownerAuth,	/* in, out */
						 UINT32 * pubEndorsementKeySize,	/* out */
						 BYTE ** pubEndorsementKey	/* out */
	    );

	TSS_RESULT TCSP_CreateRevocableEndorsementKeyPair_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
								   TPM_NONCE antiReplay,	/* in */
								   UINT32 endorsementKeyInfoSize,	/* in */
								   BYTE * endorsementKeyInfo,	/* in */
								   TSS_BOOL genResetAuth,	/* in */
								   TPM_DIGEST * eKResetAuth,	/* in, out */
								   UINT32 * endorsementKeySize,	/* out */
								   BYTE ** endorsementKey,	/* out */
								   TPM_DIGEST * checksum	/* out */
	    );

	TSS_RESULT TCSP_RevokeEndorsementKeyPair_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
							  TPM_DIGEST EKResetAuth	/* in */
	    );

	TSS_RESULT TCSP_SelfTestFull_Internal(TCS_CONTEXT_HANDLE hContext	/* in */
	    );

	TSS_RESULT TCSP_CertifySelfTest_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						  TCS_KEY_HANDLE keyHandle,	/* in */
						  TCPA_NONCE antiReplay,	/* in */
						  TPM_AUTH * privAuth,	/* in, out */
						  UINT32 * sigSize,	/* out */
						  BYTE ** sig	/* out */
	    );

	TSS_RESULT TCSP_GetTestResult_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						UINT32 * outDataSize,	/* out */
						BYTE ** outData	/* out */
	    );

	TSS_RESULT TCSP_OwnerSetDisable_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						  TSS_BOOL disableState,	/* in */
						  TPM_AUTH * ownerAuth	/* in, out */
	    );

	TSS_RESULT TCSP_ResetLockValue_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						TPM_AUTH * ownerAuth	/* in, out */
	    );

	TSS_RESULT TCSP_OwnerClear_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					     TPM_AUTH * ownerAuth	/* in, out */
	    );

	TSS_RESULT TCSP_DisableOwnerClear_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						    TPM_AUTH * ownerAuth	/* in, out */
	    );

	TSS_RESULT TCSP_ForceClear_Internal(TCS_CONTEXT_HANDLE hContext	/* in */
	    );

	TSS_RESULT TCSP_DisableForceClear_Internal(TCS_CONTEXT_HANDLE hContext	/* in */
	    );

	TSS_RESULT TCSP_PhysicalPresence_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						TCPA_PHYSICAL_PRESENCE fPhysicalPresence /* in */
	    );

	TSS_RESULT TCSP_PhysicalDisable_Internal(TCS_CONTEXT_HANDLE hContext	/* in */
	    );

	TSS_RESULT TCSP_PhysicalEnable_Internal(TCS_CONTEXT_HANDLE hContext	/* in */
	    );

	TSS_RESULT TCSP_PhysicalSetDeactivated_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
							 TSS_BOOL state	/* in */
	    );

	TSS_RESULT TCSP_SetTempDeactivated_Internal(TCS_CONTEXT_HANDLE hContext	/* in */
	    );

	TSS_RESULT TCSP_SetTempDeactivated2_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						TPM_AUTH * operatorAuth			/* in, out */
	    );

	TSS_RESULT TCSP_FieldUpgrade_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					       UINT32 dataInSize,	/* in */
					       BYTE * dataIn,	/* in */
					       UINT32 * dataOutSize,	/* out */
					       BYTE ** dataOut,	/* out */
					       TPM_AUTH * ownerAuth	/* in, out */
	    );

	TSS_RESULT TCSP_SetRedirection_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						 TCS_KEY_HANDLE keyHandle,	/* in */
						 UINT32 c1,	/* in */
						 UINT32 c2,	/* in */
						 TPM_AUTH * privAuth	/* in, out */
	    );

	TSS_RESULT TCSP_CreateMaintenanceArchive_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
							   TSS_BOOL generateRandom,	/* in */
							   TPM_AUTH * ownerAuth,	/* in, out */
							   UINT32 * randomSize,	/* out */
							   BYTE ** random,	/* out */
							   UINT32 * archiveSize,	/* out */
							   BYTE ** archive	/* out */
	    );

	TSS_RESULT TCSP_LoadMaintenanceArchive_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
							 UINT32 dataInSize,	/* in */
							 BYTE * dataIn,	/* in */
							 TPM_AUTH * ownerAuth,	/* in, out */
							 UINT32 * dataOutSize,	/* out */
							 BYTE ** dataOut	/* out */
	    );

	TSS_RESULT TCSP_KillMaintenanceFeature_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
							 TPM_AUTH * ownerAuth	/* in, out */
	    );

	TSS_RESULT TCSP_LoadManuMaintPub_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						   TCPA_NONCE antiReplay,	/* in */
						   UINT32 PubKeySize,	/* in */
						   BYTE * PubKey,	/* in */
						   TCPA_DIGEST * checksum	/* out */
	    );

	TSS_RESULT TCSP_ReadManuMaintPub_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						   TCPA_NONCE antiReplay,	/* in */
						   TCPA_DIGEST * checksum	/* out */
	    );
        TSS_RESULT TCSP_Reset_Internal(TCS_CONTEXT_HANDLE hContext
            );
	TSS_RESULT TCSP_DaaJoin_internal(TCS_CONTEXT_HANDLE hContext, /* in */
					 TPM_HANDLE handle, /* in */
					 BYTE stage,               /* in */
					 UINT32 inputSize0,   /* in */
					 BYTE *inputData0,   /* in */
					 UINT32 inputSize1, /* in */
					 BYTE *inputData1, /* in */
					 TPM_AUTH * ownerAuth,   /* in, out */
					 UINT32 *outputSize, /* out */
					 BYTE **outputData  /* out */
	    );

	TSS_RESULT TCSP_DaaSign_internal(TCS_CONTEXT_HANDLE hContext, /* in */
					 TPM_HANDLE handle, /* in */
					 BYTE stage,               /* in */
					 UINT32 inputSize0,   /* in */
					 BYTE *inputData0,   /* in */
					 UINT32 inputSize1, /* in */
					 BYTE *inputData1, /* in */
					 TPM_AUTH * ownerAuth,   /* in, out */
					 UINT32 *outputSize, /* out */
					 BYTE **outputData  /* out */
	    );


	TSS_RESULT TCSP_ReadCounter_Internal(TCS_CONTEXT_HANDLE    hContext,
					     TSS_COUNTER_ID        idCounter,
					     TPM_COUNTER_VALUE*    counterValue
	);

	TSS_RESULT TCSP_CreateCounter_Internal(TCS_CONTEXT_HANDLE    hContext,
					       UINT32                LabelSize,
					       BYTE*                 pLabel,
					       TPM_ENCAUTH           CounterAuth,
					       TPM_AUTH*             pOwnerAuth,
					       TSS_COUNTER_ID*       idCounter,
					       TPM_COUNTER_VALUE*    counterValue
	);

	TSS_RESULT TCSP_IncrementCounter_Internal(TCS_CONTEXT_HANDLE    hContext,
						  TSS_COUNTER_ID        idCounter,
						  TPM_AUTH*             pCounterAuth,
						  TPM_COUNTER_VALUE*    counterValue
	);

	TSS_RESULT TCSP_ReleaseCounter_Internal(TCS_CONTEXT_HANDLE    hContext,
						TSS_COUNTER_ID        idCounter,
						TPM_AUTH*             pCounterAuth
	);

	TSS_RESULT TCSP_ReleaseCounterOwner_Internal(TCS_CONTEXT_HANDLE    hContext,
						     TSS_COUNTER_ID        idCounter,
						     TPM_AUTH*             pOwnerAuth
	);
	TSS_RESULT TCSP_ReadCurrentTicks_Internal(TCS_CONTEXT_HANDLE hContext,
						  UINT32*            pulCurrentTime,
						  BYTE**	     prgbCurrentTime
	);
	TSS_RESULT TCSP_TickStampBlob_Internal(TCS_CONTEXT_HANDLE hContext,
					       TCS_KEY_HANDLE     hKey,
					       TPM_NONCE*         antiReplay,
					       TPM_DIGEST*        digestToStamp,
					       TPM_AUTH*          privAuth,
					       UINT32*            pulSignatureLength,
					       BYTE**             prgbSignature,
					       UINT32*            pulTickCountLength,
					       BYTE**             prgbTickCount
	);
	TSS_RESULT TCSP_EstablishTransport_Internal(TCS_CONTEXT_HANDLE      hContext,
						    UINT32                  ulTransControlFlags,
						    TCS_KEY_HANDLE          hEncKey,
						    UINT32                  ulTransSessionInfoSize,
						    BYTE*                   rgbTransSessionInfo,
						    UINT32                  ulSecretSize,
						    BYTE*                   rgbSecret,
						    TPM_AUTH*               pEncKeyAuth,
						    TPM_MODIFIER_INDICATOR* pbLocality,
						    TCS_HANDLE*             hTransSession,
						    UINT32*                 ulCurrentTicksSize,
						    BYTE**                  prgbCurrentTicks,
						    TPM_NONCE*              pTransNonce
	);

	TSS_RESULT TCSP_ExecuteTransport_Internal(TCS_CONTEXT_HANDLE      hContext,
						  TPM_COMMAND_CODE        unWrappedCommandOrdinal,
						  UINT32                  ulWrappedCmdParamInSize,
						  BYTE*                   rgbWrappedCmdParamIn,
						  UINT32*                 pulHandleListSize,
						  TCS_HANDLE**            rghHandles,
						  TPM_AUTH*               pWrappedCmdAuth1,
						  TPM_AUTH*               pWrappedCmdAuth2,
						  TPM_AUTH*               pTransAuth,
						  UINT64*                 punCurrentTicks,
						  TPM_MODIFIER_INDICATOR* pbLocality,
						  TPM_RESULT*             pulWrappedCmdReturnCode,
						  UINT32*                 ulWrappedCmdParamOutSize,
						  BYTE**                  rgbWrappedCmdParamOut
	);
	TSS_RESULT TCSP_ReleaseTransportSigned_Internal(TCS_CONTEXT_HANDLE      hContext,
							TCS_KEY_HANDLE          hSignatureKey,
							TPM_NONCE*              AntiReplayNonce,
							TPM_AUTH*               pKeyAuth,
							TPM_AUTH*               pTransAuth,
							TPM_MODIFIER_INDICATOR* pbLocality,
							UINT32*                 pulCurrentTicksSize,
							BYTE**                  prgbCurrentTicks,
							UINT32*                 pulSignatureSize,
							BYTE**                  prgbSignature
	);

	TSS_RESULT TCSP_NV_DefineOrReleaseSpace_Internal(TCS_CONTEXT_HANDLE	hContext, 	/* in */
							 UINT32			cPubInfoSize,	/* in */
							 BYTE* 			pPubInfo,	/* in */
							 TPM_ENCAUTH 		encAuth, 	/* in */
							 TPM_AUTH* 		pAuth	/* in, out */
	);

	TSS_RESULT TCSP_NV_WriteValue_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
					       TSS_NV_INDEX		hNVStore,	/* in */
					       UINT32 			offset,		/* in */
					       UINT32 			ulDataLength,	/* in */
					       BYTE* 			rgbDataToWrite,	/* in */
					       TPM_AUTH* 		privAuth	/* in, out */
	);

	TSS_RESULT TCSP_NV_WriteValueAuth_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
						   TSS_NV_INDEX 	hNVStore,	/* in */
						   UINT32 		offset,		/* in */
						   UINT32		ulDataLength,	/* in */
						   BYTE*		rgbDataToWrite,	/* in */
						   TPM_AUTH*		NVAuth	/* in, out */
	);

	TSS_RESULT TCSP_NV_ReadValue_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
					      TSS_NV_INDEX hNVStore,	/* in */
					      UINT32 offset,		/* in */
					      UINT32* pulDataLength,	/* in, out */
					      TPM_AUTH* privAuth,	/* in, out */
					      BYTE** rgbDataRead 	/* out */
	);

	TSS_RESULT TCSP_NV_ReadValueAuth_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
					          TSS_NV_INDEX		hNVStore,	/* in */
						  UINT32		offset,		/* in */
						  UINT32*		pulDataLength,	/* in, out */
						  TPM_AUTH*		NVAuth,		/* in, out */
						  BYTE**		rgbDataRead	/* out */
	);

	TSS_RESULT TCSP_SetOrdinalAuditStatus_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
						       TPM_AUTH*		ownerAuth,	/* in, out */
						       UINT32			ulOrdinal,	/* in */
						       TSS_BOOL			bAuditState	/* in */
	);

	TSS_RESULT TCSP_GetAuditDigest_Internal(TCS_CONTEXT_HANDLE	hContext,		/* in */
						UINT32			startOrdinal,		/* in */
						TPM_DIGEST*		auditDigest,		/* out */
						UINT32*			counterValueSize,	/* out */
						BYTE**			counterValue,		/* out */
						TSS_BOOL*		more,			/* out */
						UINT32*			ordSize,		/* out */
						UINT32**		ordList			/* out */
	);

	TSS_RESULT TCSP_GetAuditDigestSigned_Internal(TCS_CONTEXT_HANDLE	hContext,		/* in */
						      TCS_KEY_HANDLE		keyHandle,		/* in */
						      TSS_BOOL			closeAudit,		/* in */
						      TPM_NONCE			antiReplay,		/* in */
						      TPM_AUTH*			privAuth,		/* in, out */
						      UINT32*			counterValueSize,	/* out */
						      BYTE**			counterValue,		/* out */
						      TPM_DIGEST*		auditDigest,		/* out */
						      TPM_DIGEST*		ordinalDigest,		/* out */
						      UINT32*			sigSize,		/* out */
						      BYTE**			sig			/* out */
	);

	TSS_RESULT TCSP_SetOperatorAuth_Internal(TCS_CONTEXT_HANDLE	hContext,		/* in */
						 TCPA_SECRET*           operatorAuth		/* in */
	);

	TSS_RESULT TCSP_OwnerReadInternalPub_Internal(TCS_CONTEXT_HANDLE	hContext, /* in */
						      TCS_KEY_HANDLE 	hKey, 		/* in */
						      TPM_AUTH*		pOwnerAuth, 	/*in, out*/
						      UINT32* 		punPubKeySize,	/* out */
						      BYTE**		ppbPubKeyData	/* out */
	);

	TSS_RESULT TCSP_Delegate_Manage_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
						 TPM_FAMILY_ID		familyID,	/* in */
						 TPM_FAMILY_OPERATION	opFlag,		/* in */
						 UINT32			opDataSize,	/* in */
						 BYTE*			opData,		/* in */
						 TPM_AUTH*		ownerAuth,	/* in, out */
						 UINT32*		retDataSize,	/* out */
						 BYTE**			retData		/* out */
	);

	TSS_RESULT TCSP_Delegate_CreateKeyDelegation_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
							      TCS_KEY_HANDLE		hKey,		/* in */
							      UINT32			publicInfoSize,	/* in */
							      BYTE*			publicInfo,	/* in */
							      TPM_ENCAUTH*		encDelAuth,	/* in */
							      TPM_AUTH*			keyAuth,	/* in, out */
							      UINT32*			blobSize,	/* out */
							      BYTE**			blob		/* out */
	);

	TSS_RESULT TCSP_Delegate_CreateOwnerDelegation_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
								TSS_BOOL		increment,	/* in */
								UINT32			publicInfoSize,	/* in */
								BYTE*			publicInfo,	/* in */
								TPM_ENCAUTH*		encDelAuth,	/* in */
								TPM_AUTH*		ownerAuth,	/* in, out */
								UINT32*			blobSize,	/* out */
								BYTE**			blob		/* out */
	);

	TSS_RESULT TCSP_Delegate_LoadOwnerDelegation_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
							      TPM_DELEGATE_INDEX	index,		/* in */
							      UINT32			blobSize,	/* in */
							      BYTE*			blob,		/* in */
							      TPM_AUTH*			ownerAuth	/* in, out */
	);

	TSS_RESULT TCSP_Delegate_ReadTable_Internal(TCS_CONTEXT_HANDLE	hContext,		/* in */
						    UINT32*		pulFamilyTableSize,	/* out */
						    BYTE**		ppFamilyTable,		/* out */
						    UINT32*		pulDelegateTableSize,	/* out */
						    BYTE**		ppDelegateTable		/* out */
	);

	TSS_RESULT TCSP_Delegate_UpdateVerificationCount_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
								  UINT32		inputSize,	/* in */
								  BYTE*			input,		/* in */
								  TPM_AUTH*		ownerAuth,	/* in, out */
								  UINT32*		outputSize,	/* out */
								  BYTE**		output		/* out */
	);

	TSS_RESULT TCSP_Delegate_VerifyDelegation_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
							   UINT32		delegateSize,	/* in */
							   BYTE*		delegate	/* in */
	);

	TSS_RESULT TCSP_CMK_SetRestrictions_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
						     TSS_CMK_DELEGATE	Restriction,	/* in */
						     TPM_AUTH*		ownerAuth	/* in */
	);

	TSS_RESULT TCSP_CMK_ApproveMA_Internal(TCS_CONTEXT_HANDLE	hContext,		/* in */
					       TPM_DIGEST		migAuthorityDigest,	/* in */
					       TPM_AUTH*		ownerAuth,		/* in, out */
					       TPM_HMAC*		HmacMigAuthDigest	/* out */
	);

	TSS_RESULT TCSP_CMK_CreateKey_Internal(TCS_CONTEXT_HANDLE	hContext,		/* in */
					       TCS_KEY_HANDLE		hWrappingKey,		/* in */
					       TPM_ENCAUTH		KeyUsageAuth,		/* in */
					       TPM_HMAC			MigAuthApproval,	/* in */
					       TPM_DIGEST		MigAuthorityDigest,	/* in */
					       UINT32*			keyDataSize,		/* in, out */
					       BYTE**			prgbKeyData,		/* in, out */
					       TPM_AUTH*		pAuth			/* in, out */
	);

	TSS_RESULT TCSP_CMK_CreateTicket_Internal(TCS_CONTEXT_HANDLE	hContext,		/* in */
						  UINT32		PublicVerifyKeySize,	/* in */
						  BYTE*			PublicVerifyKey,	/* in */
						  TPM_DIGEST		SignedData,		/* in */
						  UINT32		SigValueSize,		/* in */
						  BYTE*			SigValue,		/* in */
						  TPM_AUTH*		pOwnerAuth,		/* in, out */
						  TPM_HMAC*		SigTicket		/* out */
	);

	TSS_RESULT TCSP_CMK_CreateBlob_Internal(TCS_CONTEXT_HANDLE	hContext,		/* in */
						TCS_KEY_HANDLE		parentHandle,		/* in */
						TSS_MIGRATE_SCHEME	migrationType,		/* in */
						UINT32			MigrationKeyAuthSize,	/* in */
						BYTE*			MigrationKeyAuth,	/* in */
						TPM_DIGEST		PubSourceKeyDigest,	/* in */
						UINT32			msaListSize,		/* in */
						BYTE*			msaList,		/* in */
						UINT32			restrictTicketSize,	/* in */
						BYTE*			restrictTicket,		/* in */
						UINT32			sigTicketSize,		/* in */
						BYTE*			sigTicket,		/* in */
						UINT32			encDataSize,		/* in */
						BYTE*			encData,		/* in */
						TPM_AUTH*		parentAuth,		/* in, out */
						UINT32*			randomSize,		/* out */
						BYTE**			random,			/* out */
						UINT32*			outDataSize,		/* out */
						BYTE**			outData			/* out */
	);

	TSS_RESULT TCSP_CMK_ConvertMigration_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
						      TCS_KEY_HANDLE		parentHandle,	/* in */
						      TPM_CMK_AUTH		restrictTicket,	/* in */
						      TPM_HMAC			sigTicket,	/* in */
						      UINT32			keyDataSize,	/* in */
						      BYTE*			prgbKeyData,	/* in */
						      UINT32			msaListSize,	/* in */
						      BYTE*			msaList,	/* in */
						      UINT32			randomSize,	/* in */
						      BYTE*			random,		/* in */
						      TPM_AUTH*			parentAuth,	/* in, out */
						      UINT32*			outDataSize,	/* out */
						      BYTE**			outData		/* out */
	);
	TSS_RESULT TCSP_FlushSpecific_Internal(TCS_CONTEXT_HANDLE hContext,        /* in */
					       TCS_HANDLE hResHandle,      /* in */
					       TPM_RESOURCE_TYPE resourceType /* in */
	);

	TSS_RESULT TCSP_KeyControlOwner_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					         TCS_KEY_HANDLE hKey,		/* in */
					         UINT32 ulPubKeyLength,		/* in */
					         BYTE* rgbPubKey,		/* in */
					         UINT32 attribName,		/* in */
					         TSS_BOOL attribValue,		/* in */
					         TPM_AUTH* pOwnerAuth,		/* in,out */
					         TSS_UUID* pUuidData		/* out */
	);

	TSS_RESULT TCSP_DSAP_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				      TPM_ENTITY_TYPE entityType,	/* in */
				      TCS_KEY_HANDLE hKey,		/* in */
				      TPM_NONCE *nonceOddDSAP,		/* in */
				      UINT32 entityValueSize,		/* in */
				      BYTE* entityValue,		/* in */
				      TCS_AUTHHANDLE *authHandle,	/* out */
				      TPM_NONCE *nonceEven,		/* out */
				      TPM_NONCE *nonceEvenDSAP		/* out */
	);

#endif /*_TCS_UTILS_H_ */
