
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2007
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


void
migdata_free(void *data)
{
	struct tr_migdata_obj *migdata = (struct tr_migdata_obj *)data;

	free(migdata);
}

TSS_BOOL
obj_is_migdata(TSS_HOBJECT hObject)
{
	TSS_BOOL answer = FALSE;

	if ((obj_list_get_obj(&migdata_list, hObject))) {
		answer = TRUE;
		obj_list_put(&migdata_list);
	}

	return answer;
}

TSS_RESULT
obj_migdata_add(TSS_HCONTEXT hContext, TSS_HOBJECT *phObject)
{
	TSS_RESULT result;
	struct tr_migdata_obj *migdata = calloc(1, sizeof(struct tr_migdata_obj));

	if (migdata == NULL) {
		LogError("malloc of %zd bytes failed.",	sizeof(struct tr_migdata_obj));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	if ((result = obj_list_add(&migdata_list, hContext, 0, migdata, phObject))) {
		free(migdata);
		return result;
	}

	return TSS_SUCCESS;
}

TSS_RESULT
obj_migdata_remove(TSS_HMIGDATA hMigData, TSS_HCONTEXT hContext)
{
	TSS_RESULT result;

	if ((result = obj_list_remove(&migdata_list, &migdata_free, hMigData, hContext)))
		return result;

	return TSS_SUCCESS;
}

TSS_RESULT
obj_migdata_get_tsp_context(TSS_HMIGDATA hMigData, TSS_HCONTEXT *hContext)
{
	struct tsp_object *obj;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	*hContext = obj->tspContext;

	obj_list_put(&migdata_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_migdata_set_migrationblob(TSS_HMIGDATA hMigData, UINT32 whichOne, UINT32 blobSize, BYTE *blob)
{
	TSS_RESULT result;

	switch (whichOne) {
	case TSS_MIGATTRIB_MIG_MSALIST_PUBKEY_BLOB:
		result = obj_migdata_set_msa_pubkey(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_MIG_AUTHORITY_PUBKEY_BLOB:
		result = obj_migdata_set_ma_pubkey(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_MIG_DESTINATION_PUBKEY_BLOB:
		result = obj_migdata_set_dest_pubkey(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_MIG_SOURCE_PUBKEY_BLOB:
		result = obj_migdata_set_src_pubkey(hMigData, blobSize, blob);
		break;
	default:
		result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	return result;
}

TSS_RESULT
obj_migdata_get_migrationblob(TSS_HMIGDATA hMigData, UINT32 whichOne, UINT32 *blobSize, BYTE **blob)
{
	TSS_RESULT result;

	switch (whichOne) {
	case TSS_MIGATTRIB_MIG_XOR_BLOB:
		result = obj_migdata_get_blob(hMigData, blobSize, blob);
		break;
	default:
		result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	return result;
}

TSS_RESULT
obj_migdata_set_authoritydata(TSS_HMIGDATA hMigData, UINT32 whichOne, UINT32 blobSize, BYTE *blob)
{
	TSS_RESULT result;

	switch (whichOne) {
	case TSS_MIGATTRIB_AUTHORITY_DIGEST:
		result = obj_migdata_set_msa_digest(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_AUTHORITY_APPROVAL_HMAC:
		result = obj_migdata_set_msa_hmac(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_AUTHORITY_MSALIST:
		result = obj_migdata_set_msa_list(hMigData, blobSize, blob);
		break;
	default:
		result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	return result;
}

TSS_RESULT
obj_migdata_get_authoritydata(TSS_HMIGDATA hMigData, UINT32 whichOne, UINT32 *blobSize, BYTE **blob)
{
	TSS_RESULT result;

	switch (whichOne) {
	case TSS_MIGATTRIB_AUTHORITY_DIGEST:
		result = obj_migdata_get_msa_digest(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_AUTHORITY_APPROVAL_HMAC:
		result = obj_migdata_get_msa_hmac(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_AUTHORITY_MSALIST:
		result = obj_migdata_get_msa_list(hMigData, blobSize, blob);
		break;
	default:
		result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	return result;
}

TSS_RESULT
obj_migdata_set_migauthdata(TSS_HMIGDATA hMigData, UINT32 whichOne, UINT32 blobSize, BYTE *blob)
{
	TSS_RESULT result;

	switch (whichOne) {
	case TSS_MIGATTRIB_MIG_AUTH_AUTHORITY_DIGEST:
		result = obj_migdata_set_ma_digest(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_MIG_AUTH_DESTINATION_DIGEST:
		result = obj_migdata_set_dest_digest(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_MIG_AUTH_SOURCE_DIGEST:
		result = obj_migdata_set_src_digest(hMigData, blobSize, blob);
		break;
	default:
		result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	return result;
}

TSS_RESULT
obj_migdata_get_migauthdata(TSS_HMIGDATA hMigData, UINT32 whichOne, UINT32 *blobSize, BYTE **blob)
{
	TSS_RESULT result;

	switch (whichOne) {
	case TSS_MIGATTRIB_MIG_AUTH_AUTHORITY_DIGEST:
		result = obj_migdata_get_ma_digest(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_MIG_AUTH_DESTINATION_DIGEST:
		result = obj_migdata_get_dest_digest(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_MIG_AUTH_SOURCE_DIGEST:
		result = obj_migdata_get_src_digest(hMigData, blobSize, blob);
		break;
	default:
		result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	return result;
}

TSS_RESULT
obj_migdata_set_ticketdata(TSS_HMIGDATA hMigData, UINT32 whichOne, UINT32 blobSize, BYTE *blob)
{
	TSS_RESULT result;

	switch (whichOne) {
	case TSS_MIGATTRIB_TICKET_SIG_DIGEST:
		result = obj_migdata_set_sig_data(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_TICKET_SIG_VALUE:
		result = obj_migdata_set_sig_value(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_TICKET_SIG_TICKET:
		result = obj_migdata_set_sig_ticket(hMigData, blobSize, blob);
		break;
	case TSS_MIGATTRIB_TICKET_RESTRICT_TICKET:
		result = obj_migdata_set_cmk_auth(hMigData, blobSize, blob);
		break;
	default:
		result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	return result;
}

TSS_RESULT
obj_migdata_get_ticketdata(TSS_HMIGDATA hMigData, UINT32 whichOne, UINT32 *blobSize, BYTE **blob)
{
	TSS_RESULT result;

	switch (whichOne) {
	case TSS_MIGATTRIB_TICKET_SIG_TICKET:
		result = obj_migdata_get_sig_ticket(hMigData, blobSize, blob);
		break;
	default:
		result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	return result;
}

TSS_RESULT
obj_migdata_set_ticket_blob(TSS_HMIGDATA hMigData, UINT32 migTicketSize, BYTE *migTicket)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	migdata->migTicketSize = 0;
	free(migdata->migTicket);
	if ((migdata->migTicket = malloc(migTicketSize)) == NULL) {
		LogError("malloc of %u bytes failed.", migTicketSize);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(migdata->migTicket, migTicket, migTicketSize);
	migdata->migTicketSize = migTicketSize;

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_get_ticket_blob(TSS_HMIGDATA hMigData, UINT32 *migTicketSize, BYTE **migTicket)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((*migTicket = calloc_tspi(obj->tspContext, migdata->migTicketSize)) == NULL) {
		LogError("malloc of %u bytes failed.", migdata->migTicketSize);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(*migTicket, migdata->migTicket, migdata->migTicketSize);
	*migTicketSize = migdata->migTicketSize;

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_set_msa_list(TSS_HMIGDATA hMigData, UINT32 msaListSize, BYTE *msaList)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	UINT32 i, count, size;
	TPM_DIGEST *digest;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	count = msaListSize / sizeof(digest->digest);
	size = count * sizeof(*digest);

	migdata->msaList.MSAlist = 0;
	free(migdata->msaList.migAuthDigest);
	if ((migdata->msaList.migAuthDigest = malloc(size)) == NULL) {
		LogError("malloc of %u bytes failed.", size);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	digest = migdata->msaList.migAuthDigest;
	for (i = 0; i < count; i++) {
		memcpy(digest->digest, msaList, sizeof(digest->digest));
		msaList += sizeof(digest->digest);
		digest++;
	}
	migdata->msaList.MSAlist = count;

	result = obj_migdata_calc_msa_digest(migdata);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_get_msa_list(TSS_HMIGDATA hMigData, UINT32 *size, BYTE **msaList)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	UINT32 i;
	TPM_DIGEST *digest;
	BYTE *tmpMsaList;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	*size = migdata->msaList.MSAlist * sizeof(migdata->msaList.migAuthDigest->digest);
	if ((*msaList = calloc_tspi(obj->tspContext, *size)) == NULL) {
		LogError("malloc of %u bytes failed.", *size);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	tmpMsaList = *msaList;
	digest = migdata->msaList.migAuthDigest;
	for (i = 0; i < migdata->msaList.MSAlist; i++) {
		memcpy(tmpMsaList, digest->digest, sizeof(digest->digest));
		tmpMsaList += sizeof(digest->digest);
		digest++;
	}

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_set_msa_pubkey(TSS_HMIGDATA hMigData, UINT32 blobSize, BYTE *pubKeyBlob)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	UINT32 size;
	TPM_DIGEST msaDigest;
	TPM_DIGEST *digest;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((result = obj_migdata_calc_pubkey_digest(blobSize, pubKeyBlob, &msaDigest)))
		goto done;

	size = (migdata->msaList.MSAlist + 1) * sizeof(*digest);
	if ((migdata->msaList.migAuthDigest = realloc(migdata->msaList.migAuthDigest, size)) == NULL) {
		LogError("malloc of %u bytes failed.", size);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	digest = migdata->msaList.migAuthDigest + migdata->msaList.MSAlist;
	*digest = msaDigest;
	migdata->msaList.MSAlist++;

	result = obj_migdata_calc_msa_digest(migdata);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_set_msa_digest(TSS_HMIGDATA hMigData, UINT32 digestSize, BYTE *digest)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if (digestSize != sizeof(migdata->msaDigest.digest)) {
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}
	memcpy(migdata->msaDigest.digest, digest, sizeof(migdata->msaDigest.digest));

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_get_msa_digest(TSS_HMIGDATA hMigData, UINT32 *digestSize, BYTE **digest)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((*digest = calloc_tspi(obj->tspContext, sizeof(migdata->msaDigest.digest))) == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(migdata->msaDigest.digest));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(*digest, migdata->msaDigest.digest, sizeof(migdata->msaDigest.digest));
	*digestSize = sizeof(migdata->msaDigest.digest);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_get_msa_list_blob(TSS_HMIGDATA hMigData, UINT32 *blobSize, BYTE **msaListBlob)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	UINT64 offset;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	offset = 0;
	Trspi_LoadBlob_MSA_COMPOSITE(&offset, NULL, &migdata->msaList);

	*blobSize = offset;
	if ((*msaListBlob = calloc_tspi(obj->tspContext, *blobSize)) == NULL) {
		LogError("malloc of %u bytes failed.", *blobSize);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	offset = 0;
	Trspi_LoadBlob_MSA_COMPOSITE(&offset, *msaListBlob, &migdata->msaList);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_set_msa_hmac(TSS_HMIGDATA hMigData, UINT32 hmacSize, BYTE *hmac)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if (hmacSize != sizeof(migdata->msaHmac.digest)) {
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}
	memcpy(migdata->msaHmac.digest, hmac, sizeof(migdata->msaHmac.digest));

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_get_msa_hmac(TSS_HMIGDATA hMigData, UINT32 *hmacSize, BYTE **hmac)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((*hmac = calloc_tspi(obj->tspContext, sizeof(migdata->msaHmac.digest))) == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(migdata->msaHmac.digest));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(*hmac, migdata->msaHmac.digest, sizeof(migdata->msaHmac.digest));
	*hmacSize = sizeof(migdata->msaHmac.digest);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_set_ma_pubkey(TSS_HMIGDATA hMigData, UINT32 blobSize, BYTE *pubKeyBlob)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TPM_DIGEST pubKeyDigest;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((result = obj_migdata_calc_pubkey_digest(blobSize, pubKeyBlob, &pubKeyDigest)))
		goto done;

	migdata->maDigest = pubKeyDigest;

	obj_migdata_calc_sig_data_digest(migdata);

done:
	obj_list_put(&migdata_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_migdata_set_ma_digest(TSS_HMIGDATA hMigData, UINT32 digestSize, BYTE *digest)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if (digestSize != sizeof(migdata->maDigest.digest)) {
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}
	memcpy(migdata->maDigest.digest, digest, sizeof(migdata->maDigest.digest));

	obj_migdata_calc_sig_data_digest(migdata);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_get_ma_digest(TSS_HMIGDATA hMigData, UINT32 *digestSize, BYTE **digest)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((*digest = calloc_tspi(obj->tspContext, sizeof(migdata->maDigest.digest))) == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(migdata->maDigest.digest));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(*digest, migdata->maDigest.digest, sizeof(migdata->maDigest.digest));
	*digestSize = sizeof(migdata->maDigest.digest);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_set_dest_pubkey(TSS_HMIGDATA hMigData, UINT32 blobSize, BYTE *pubKeyBlob)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TPM_DIGEST pubKeyDigest;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((result = obj_migdata_calc_pubkey_digest(blobSize, pubKeyBlob, &pubKeyDigest)))
		goto done;

	migdata->destDigest = pubKeyDigest;

	obj_migdata_calc_sig_data_digest(migdata);

done:
	obj_list_put(&migdata_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_migdata_set_dest_digest(TSS_HMIGDATA hMigData, UINT32 digestSize, BYTE *digest)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if (digestSize != sizeof(migdata->destDigest.digest)) {
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}
	memcpy(migdata->destDigest.digest, digest, sizeof(migdata->destDigest.digest));

	obj_migdata_calc_sig_data_digest(migdata);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_get_dest_digest(TSS_HMIGDATA hMigData, UINT32 *digestSize, BYTE **digest)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((*digest = calloc_tspi(obj->tspContext, sizeof(migdata->destDigest.digest))) == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(migdata->destDigest.digest));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(*digest, migdata->destDigest.digest, sizeof(migdata->destDigest.digest));
	*digestSize = sizeof(migdata->destDigest.digest);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_set_src_pubkey(TSS_HMIGDATA hMigData, UINT32 blobSize, BYTE *pubKeyBlob)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TPM_DIGEST pubKeyDigest;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((result = obj_migdata_calc_pubkey_digest(blobSize, pubKeyBlob, &pubKeyDigest)))
		goto done;

	migdata->srcDigest = pubKeyDigest;

	obj_migdata_calc_sig_data_digest(migdata);

done:
	obj_list_put(&migdata_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_migdata_set_src_digest(TSS_HMIGDATA hMigData, UINT32 digestSize, BYTE *digest)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if (digestSize != sizeof(migdata->srcDigest.digest)) {
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}
	memcpy(migdata->srcDigest.digest, digest, sizeof(migdata->srcDigest.digest));

	obj_migdata_calc_sig_data_digest(migdata);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_get_src_digest(TSS_HMIGDATA hMigData, UINT32 *digestSize, BYTE **digest)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((*digest = calloc_tspi(obj->tspContext, sizeof(migdata->srcDigest.digest))) == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(migdata->srcDigest.digest));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(*digest, migdata->srcDigest.digest, sizeof(migdata->srcDigest.digest));
	*digestSize = sizeof(migdata->srcDigest.digest);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_set_cmk_auth(TSS_HMIGDATA hMigData, UINT32 digestsSize, BYTE *digests)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if (digestsSize != (sizeof(migdata->maDigest.digest) +
				sizeof(migdata->destDigest.digest) +
				sizeof(migdata->srcDigest.digest))) {
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}
	memcpy(migdata->maDigest.digest, digests, sizeof(migdata->maDigest.digest));
	digests += sizeof(migdata->maDigest.digest);
	memcpy(migdata->destDigest.digest, digests, sizeof(migdata->destDigest.digest));
	digests += sizeof(migdata->destDigest.digest);
	memcpy(migdata->srcDigest.digest, digests, sizeof(migdata->srcDigest.digest));

	obj_migdata_calc_sig_data_digest(migdata);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_get_cmk_auth(TSS_HMIGDATA hMigData, TPM_CMK_AUTH *cmkAuth)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	cmkAuth->migrationAuthorityDigest = migdata->maDigest;
	cmkAuth->destinationKeyDigest = migdata->destDigest;
	cmkAuth->sourceKeyDigest = migdata->srcDigest;

	obj_list_put(&migdata_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_migdata_get_cmk_auth_blob(TSS_HMIGDATA hMigData, UINT32 *blobSize, BYTE **cmkAuthBlob)
{
	struct tsp_object *obj;
	TPM_CMK_AUTH cmkAuth;
	UINT64 offset;
	TSS_RESULT result;

	if ((result = obj_migdata_get_cmk_auth(hMigData, &cmkAuth)))
		return result;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	offset = 0;
	Trspi_LoadBlob_CMK_AUTH(&offset, NULL, &cmkAuth);

	*blobSize = offset;
	if ((*cmkAuthBlob = calloc_tspi(obj->tspContext, *blobSize)) == NULL) {
		LogError("malloc of %u bytes failed.", *blobSize);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	offset = 0;
	Trspi_LoadBlob_CMK_AUTH(&offset, *cmkAuthBlob, &cmkAuth);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_set_sig_data(TSS_HMIGDATA hMigData, UINT32 sigDataSize, BYTE *sigData)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if (sigDataSize != sizeof(migdata->sigData.digest)) {
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}
	memcpy(migdata->sigData.digest, sigData, sizeof(migdata->sigData.digest));

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_get_sig_data(TSS_HMIGDATA hMigData, UINT32 *sigDataSize, BYTE **sigData)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((*sigData = calloc_tspi(obj->tspContext, sizeof(migdata->sigData.digest))) == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(migdata->sigData.digest));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(*sigData, migdata->sigData.digest, sizeof(migdata->sigData.digest));
	*sigDataSize = sizeof(migdata->sigData.digest);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_set_sig_value(TSS_HMIGDATA hMigData, UINT32 sigValueSize, BYTE *sigValue)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	migdata->sigValueSize = 0;
	free(migdata->sigValue);
	if ((migdata->sigValue = malloc(sigValueSize)) == NULL) {
		LogError("malloc of %u bytes failed.", sigValueSize);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(migdata->sigValue, sigValue, sigValueSize);
	migdata->sigValueSize = sigValueSize;

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_get_sig_value(TSS_HMIGDATA hMigData, UINT32 *sigValueSize, BYTE **sigValue)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((*sigValue = calloc_tspi(obj->tspContext, migdata->sigValueSize)) == NULL) {
		LogError("malloc of %u bytes failed.", migdata->sigValueSize);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(*sigValue, migdata->sigValue, migdata->sigValueSize);
	*sigValueSize = migdata->sigValueSize;

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_set_sig_ticket(TSS_HMIGDATA hMigData, UINT32 sigTicketSize, BYTE *sigTicket)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if (sigTicketSize != sizeof(migdata->sigTicket.digest)) {
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}
	memcpy(migdata->sigTicket.digest, sigTicket, sizeof(migdata->sigTicket.digest));

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_get_sig_ticket(TSS_HMIGDATA hMigData, UINT32 *sigTicketSize, BYTE **sigTicket)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((*sigTicket = calloc_tspi(obj->tspContext, sizeof(migdata->sigTicket.digest))) == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(migdata->sigTicket.digest));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(*sigTicket, migdata->sigTicket.digest, sizeof(migdata->sigTicket.digest));
	*sigTicketSize = sizeof(migdata->sigTicket.digest);

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_set_blob(TSS_HMIGDATA hMigData, UINT32 blobSize, BYTE *blob)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	migdata->blobSize = 0;
	free(migdata->blob);
	if ((migdata->blob = malloc(blobSize)) == NULL) {
		LogError("malloc of %u bytes failed.", blobSize);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(migdata->blob, blob, blobSize);
	migdata->blobSize = blobSize;

done:
	obj_list_put(&migdata_list);

	return result;
}

TSS_RESULT
obj_migdata_get_blob(TSS_HMIGDATA hMigData, UINT32 *blobSize, BYTE **blob)
{
	struct tsp_object *obj;
	struct tr_migdata_obj *migdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&migdata_list, hMigData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	migdata = (struct tr_migdata_obj *)obj->data;

	if ((*blob = calloc_tspi(obj->tspContext, migdata->blobSize)) == NULL) {
		LogError("malloc of %u bytes failed.", migdata->blobSize);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(*blob, migdata->blob, migdata->blobSize);
	*blobSize = migdata->blobSize;

done:
	obj_list_put(&migdata_list);

	return result;
}


TSS_RESULT
obj_migdata_calc_pubkey_digest(UINT32 blobSize, BYTE *blob, TPM_DIGEST *digest)
{
	Trspi_HashCtx hashCtx;
	TSS_RESULT result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_HashUpdate(&hashCtx, blobSize, blob);
	result |= Trspi_HashFinal(&hashCtx, digest->digest);

	return result;
}

TSS_RESULT
obj_migdata_calc_msa_digest(struct tr_migdata_obj *migdata)
{
	Trspi_HashCtx hashCtx;
	TSS_RESULT result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_MSA_COMPOSITE(&hashCtx, &migdata->msaList);
	result |= Trspi_HashFinal(&hashCtx, migdata->msaDigest.digest);

	return result;
}

TSS_RESULT
obj_migdata_calc_sig_data_digest(struct tr_migdata_obj *migdata)
{
	Trspi_HashCtx hashCtx;
	TSS_RESULT result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, migdata->maDigest.digest);
	result |= Trspi_Hash_DIGEST(&hashCtx, migdata->destDigest.digest);
	result |= Trspi_Hash_DIGEST(&hashCtx, migdata->srcDigest.digest);
	result |= Trspi_HashFinal(&hashCtx, migdata->sigData.digest);

	return result;
}
