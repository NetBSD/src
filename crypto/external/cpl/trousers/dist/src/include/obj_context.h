
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _OBJ_CONTEXT_H_
#define _OBJ_CONTEXT_H_

#define TSS_CONTEXT_FLAGS_TRANSPORT_ENABLED		0x01
#define TSS_CONTEXT_FLAGS_TRANSPORT_DEFAULT_ENCRYPT	0x02
#define TSS_CONTEXT_FLAGS_TRANSPORT_AUTHENTIC		0x04
#define TSS_CONTEXT_FLAGS_TRANSPORT_EXCLUSIVE		0x08
#define TSS_CONTEXT_FLAGS_TRANSPORT_STATIC_AUTH		0x10
#define TSS_CONTEXT_FLAGS_TRANSPORT_ESTABLISHED		0x20
#define TSS_CONTEXT_FLAGS_TRANSPORT_MASK		0x3f

#define TSS_CONTEXT_FLAGS_TPM_VERSION_1			0x40
#define TSS_CONTEXT_FLAGS_TPM_VERSION_2			0x80
#define TSS_CONTEXT_FLAGS_TPM_VERSION_MASK		0xc0

/* structures */
struct tr_context_obj {
	TSS_FLAG silentMode, flags;
	UINT32 hashMode;
	TSS_HPOLICY policy;
	BYTE *machineName;
	UINT32 machineNameLength;
	UINT32 connection_policy, current_connection;
	struct tcs_api_table *tcs_api;
#ifdef TSS_BUILD_TRANSPORT
	/* transport session support */
	TSS_HKEY transKey;
	TPM_TRANSPORT_PUBLIC transPub;
	TPM_MODIFIER_INDICATOR transMod;
	TPM_TRANSPORT_AUTH transSecret;
	TPM_AUTH transAuth;
	TPM_TRANSPORT_LOG_IN transLogIn;
	TPM_TRANSPORT_LOG_OUT transLogOut;
	TPM_DIGEST transLogDigest;
#endif
};

/* obj_context.c */
void       __tspi_obj_context_free(void *data);
TSS_BOOL   obj_is_context(TSS_HOBJECT);
TSS_RESULT obj_context_get_policy(TSS_HCONTEXT, UINT32, TSS_HPOLICY *);
TSS_BOOL   obj_context_is_silent(TSS_HCONTEXT);
TSS_RESULT obj_context_get_machine_name(TSS_HCONTEXT, UINT32 *, BYTE **);
TSS_RESULT obj_context_get_machine_name_attrib(TSS_HCONTEXT, UINT32 *, BYTE **);
TSS_RESULT obj_context_set_machine_name(TSS_HCONTEXT, BYTE *, UINT32);
TSS_RESULT obj_context_add(TSS_HOBJECT *);
TSS_RESULT obj_context_set_mode(TSS_HCONTEXT, UINT32);
TSS_RESULT obj_context_get_mode(TSS_HCONTEXT, UINT32 *);
TSS_BOOL   obj_context_has_popups(TSS_HCONTEXT);
TSS_RESULT obj_context_get_hash_mode(TSS_HCONTEXT, UINT32 *);
TSS_RESULT obj_context_set_hash_mode(TSS_HCONTEXT, UINT32);
TSS_RESULT obj_context_get_connection_version(TSS_HCONTEXT, UINT32 *);
TSS_RESULT obj_context_set_connection_policy(TSS_HCONTEXT, UINT32);
#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT obj_context_set_transport_key(TSS_HCONTEXT, TSS_HKEY);
TSS_RESULT obj_context_transport_get_control(TSS_HCONTEXT, UINT32, UINT32 *);
TSS_RESULT obj_context_transport_set_control(TSS_HCONTEXT, UINT32);
TSS_RESULT obj_context_transport_get_mode(TSS_HCONTEXT, UINT32, UINT32 *);
TSS_RESULT obj_context_transport_set_mode(TSS_HCONTEXT, UINT32);
TSS_RESULT obj_context_transport_init(TSS_HCONTEXT);
TSS_RESULT obj_context_transport_establish(TSS_HCONTEXT, struct tr_context_obj *);
TSS_RESULT obj_context_transport_execute(TSS_HCONTEXT, TPM_COMMAND_CODE, UINT32, BYTE*, TPM_DIGEST*,
					 UINT32*, TCS_HANDLE**, TPM_AUTH*, TPM_AUTH*, UINT32*,
					 BYTE**);
TSS_RESULT obj_context_transport_close(TSS_HCONTEXT, TSS_HKEY, TSS_HPOLICY, TSS_BOOL,
				       TPM_SIGN_INFO*, UINT32*, BYTE**);
#endif
TSS_RESULT obj_context_set_tpm_version(TSS_HCONTEXT, UINT32);
TSS_RESULT obj_context_get_tpm_version(TSS_HCONTEXT, UINT32 *);
TSS_RESULT obj_context_get_loadkey_ordinal(TSS_HCONTEXT, TPM_COMMAND_CODE *);
void       obj_context_close(TSS_HCONTEXT);

struct tcs_api_table *obj_context_get_tcs_api(TSS_HCONTEXT);
#define TCS_API(c) obj_context_get_tcs_api(c)


#define CONTEXT_LIST_DECLARE		struct obj_list context_list
#define CONTEXT_LIST_DECLARE_EXTERN	extern struct obj_list context_list
#define CONTEXT_LIST_INIT()		list_init(&context_list)
#define CONTEXT_LIST_CONNECT(a,b)	obj_connectContext_list(&context_list, a, b)
#define CONTEXT_LIST_CLOSE(a)		obj_list_close(&context_list, &__tspi_obj_context_free, a)

#endif
