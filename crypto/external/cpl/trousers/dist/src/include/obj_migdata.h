
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2007
 *
 */

#ifndef _OBJ_MIGDATA_H_
#define _OBJ_MIGDATA_H_

#ifdef TSS_BUILD_CMK

/* structures */
struct tr_migdata_obj {
	/* TSS_MIGATTRIB_MIGRATIONTICKET (from AuthorizeMigrationTicket)	*/
	UINT32 migTicketSize;
	BYTE *migTicket;

	/* TSS_MIGATTRIB_AUTHORITY_DATA/TSS_MIGATTRIB_AUTHORITY_MSALIST
		- Recalculate the msaDigest
	   or
	   TSS_MIGATTRIB_MIGRATIONBLOB/TSS_MIGATTRIB_MIG_MSALIST_PUBKEY_BLOB
		- Create a digest from the pubkey blob and append to the list
		- Recalculate the msaDigest					*/
	TPM_MSA_COMPOSITE msaList;
	/* TSS_MIGATTRIB_AUTHORITY_DATA/TSS_MIGATTRIB_AUTHORITY_DIGEST		*/
	TPM_DIGEST msaDigest;
	/* TSS_MIGATTRIB_AUTHORITY_DATA/TSS_MIGATTRIB_AUTHORITY_APPROVAL_HMAC	*/
	TPM_HMAC msaHmac;

	/* TSS_MIGATTRIB_MIG_AUTH_DATA/TSS_MIGATTRIB_MIG_AUTH_AUTHORITY_DIGEST
	   or
	   TSS_MIGATTRIB_MIGRATIONBLOB/TSS_MIGATTRIB_MIG_AUTHORITY_PUBKEY_BLOB
		- Create a digest from the pubkey blob				*/
	TPM_DIGEST maDigest;
	/* TSS_MIGATTRIB_MIG_AUTH_DATA/TSS_MIGATTRIB_MIG_AUTH_DESTINATION_DIGEST
	   or
	   TSS_MIGATTRIB_MIGRATIONBLOB/TSS_MIGATTRIB_MIG_DESTINATION_PUBKEY_BLOB
		- Create a digest from the pubkey blob				*/
	TPM_DIGEST destDigest;
	/* TSS_MIGATTRIB_MIG_AUTH_DATA/TSS_MIGATTRIB_MIG_AUTH_SOURCE_DIGEST
	   or
	   TSS_MIGATTRIB_MIGRATIONBLOB/TSS_MIGATTRIB_MIG_SOURCE_PUBKEY_BLOB
		- Create a digest from the pubkey blob				*/
	TPM_DIGEST srcDigest;

	/* TSS_MIGATTRIB_TICKET_DATA/TSS_MIGATTRIB_TICKET_SIG_DIGEST		*/
	TPM_DIGEST sigData;
	/* TSS_MIGATTRIB_TICKET_DATA/TSS_MIGATTRIB_TICKET_SIG_VALUE		*/
	UINT32 sigValueSize;
	BYTE *sigValue;
	/* TSS_MIGATTRIB_TICKET_DATA/TSS_MIGATTRIB_TICKET_SIG_TICKET		*/
	TPM_HMAC sigTicket;

	/* TSS_MIGATTRIB_MIGRATIONBLOB/TSS_MIGATTRIB_MIGRATION_XOR_BLOB		*/
	UINT32 blobSize;
	BYTE *blob;
};

/* obj_migdata.c */
void       migdata_free(void *data);
TSS_BOOL   obj_is_migdata(TSS_HOBJECT);
TSS_RESULT obj_migdata_add(TSS_HCONTEXT, TSS_HOBJECT *);
TSS_RESULT obj_migdata_remove(TSS_HMIGDATA, TSS_HOBJECT);
TSS_RESULT obj_migdata_get_tsp_context(TSS_HMIGDATA, TSS_HCONTEXT *);

TSS_RESULT obj_migdata_set_migrationblob(TSS_HMIGDATA, UINT32, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_migrationblob(TSS_HMIGDATA, UINT32, UINT32 *, BYTE **);
TSS_RESULT obj_migdata_set_authoritydata(TSS_HMIGDATA, UINT32, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_authoritydata(TSS_HMIGDATA, UINT32, UINT32 *, BYTE **);
TSS_RESULT obj_migdata_set_migauthdata(TSS_HMIGDATA, UINT32, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_migauthdata(TSS_HMIGDATA, UINT32, UINT32 *, BYTE **);
TSS_RESULT obj_migdata_set_ticketdata(TSS_HMIGDATA, UINT32, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_ticketdata(TSS_HMIGDATA, UINT32, UINT32 *, BYTE **);

TSS_RESULT obj_migdata_set_ticket_blob(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_ticket_blob(TSS_HMIGDATA, UINT32 *, BYTE **);

TSS_RESULT obj_migdata_set_msa_list(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_msa_list(TSS_HMIGDATA, UINT32 *, BYTE **);
TSS_RESULT obj_migdata_set_msa_pubkey(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_set_msa_digest(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_msa_digest(TSS_HMIGDATA, UINT32 *, BYTE **);
TSS_RESULT obj_migdata_get_msa_list_blob(TSS_HMIGDATA, UINT32 *, BYTE **);

TSS_RESULT obj_migdata_set_msa_hmac(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_msa_hmac(TSS_HMIGDATA, UINT32 *, BYTE **);

TSS_RESULT obj_migdata_set_ma_pubkey(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_set_ma_digest(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_ma_digest(TSS_HMIGDATA, UINT32 *, BYTE **);
TSS_RESULT obj_migdata_set_dest_pubkey(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_set_dest_digest(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_dest_digest(TSS_HMIGDATA, UINT32 *, BYTE **);
TSS_RESULT obj_migdata_set_src_pubkey(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_set_src_digest(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_src_digest(TSS_HMIGDATA, UINT32 *, BYTE **);
TSS_RESULT obj_migdata_set_cmk_auth(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_cmk_auth(TSS_HMIGDATA, TPM_CMK_AUTH *);
TSS_RESULT obj_migdata_get_cmk_auth_blob(TSS_HMIGDATA, UINT32 *, BYTE **);

TSS_RESULT obj_migdata_set_sig_data(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_sig_data(TSS_HMIGDATA, UINT32 *, BYTE **);
TSS_RESULT obj_migdata_set_sig_value(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_sig_value(TSS_HMIGDATA, UINT32 *, BYTE **);
TSS_RESULT obj_migdata_set_sig_ticket(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_sig_ticket(TSS_HMIGDATA, UINT32 *, BYTE **);

TSS_RESULT obj_migdata_set_blob(TSS_HMIGDATA, UINT32, BYTE *);
TSS_RESULT obj_migdata_get_blob(TSS_HMIGDATA, UINT32 *, BYTE **);

TSS_RESULT obj_migdata_calc_pubkey_digest(UINT32, BYTE *, TPM_DIGEST *);
TSS_RESULT obj_migdata_calc_msa_digest(struct tr_migdata_obj *);
TSS_RESULT obj_migdata_calc_sig_data_digest(struct tr_migdata_obj *);

#define MIGDATA_LIST_DECLARE		struct obj_list migdata_list
#define MIGDATA_LIST_DECLARE_EXTERN	extern struct obj_list migdata_list
#define MIGDATA_LIST_INIT()		list_init(&migdata_list)
#define MIGDATA_LIST_CONNECT(a,b)	obj_connectContext_list(&migdata_list, a, b)
#define MIGDATA_LIST_CLOSE(a)		obj_list_close(&migdata_list, &migdata_free, a)

#else

#define obj_is_migdata(a)		FALSE

#define MIGDATA_LIST_DECLARE
#define MIGDATA_LIST_DECLARE_EXTERN
#define MIGDATA_LIST_INIT()
#define MIGDATA_LIST_CONNECT(a,b)
#define MIGDATA_LIST_CLOSE(a)

#endif

#endif
