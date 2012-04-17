
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


UINT16
get_num_pcrs(TSS_HCONTEXT tspContext)
{
	TSS_RESULT result;
	static UINT16 ret = 0;
	UINT32 subCap;
	UINT32 respSize;
	BYTE *resp;

	if (ret != 0)
		return ret;

	subCap = endian32(TPM_CAP_PROP_PCR);
	if ((result = TCS_API(tspContext)->GetTPMCapability(tspContext, TPM_CAP_PROPERTY,
							    sizeof(UINT32), (BYTE *)&subCap,
							    &respSize, &resp))) {
		if ((resp = (BYTE *)getenv("TSS_DEFAULT_NUM_PCRS")) == NULL)
			return TSS_DEFAULT_NUM_PCRS;

		/* don't set ret here, next time we may be connected */
		return atoi((char *)resp);
	}

	ret = (UINT16)Decode_UINT32(resp);
	free(resp);

	return ret;
}

TSS_RESULT
pcrs_calc_composite(TPM_PCR_SELECTION *select, TPM_PCRVALUE *arrayOfPcrs, TPM_DIGEST *digestOut)
{
	UINT32 size, index;
	BYTE mask;
	BYTE hashBlob[1024];
	UINT32 numPCRs = 0;
	UINT64 offset = 0;
	UINT64 sizeOffset = 0;

	if (select->sizeOfSelect > 0) {
		sizeOffset = 0;
		Trspi_LoadBlob_PCR_SELECTION(&sizeOffset, hashBlob, select);
		offset = sizeOffset + 4;

		for (size = 0; size < select->sizeOfSelect; size++) {
			for (index = 0, mask = 1; index < 8; index++, mask = mask << 1) {
				if (select->pcrSelect[size] & mask) {
					memcpy(&hashBlob[(numPCRs * TPM_SHA1_160_HASH_LEN) + offset],
					       arrayOfPcrs[index + (size << 3)].digest,
					       TPM_SHA1_160_HASH_LEN);
					numPCRs++;
				}
			}
		}

		if (numPCRs > 0) {
			offset += (numPCRs * TPM_SHA1_160_HASH_LEN);
			UINT32ToArray(numPCRs * TPM_SHA1_160_HASH_LEN, &hashBlob[sizeOffset]);

			return Trspi_Hash(TSS_HASH_SHA1, offset, hashBlob, digestOut->digest);
		}
	}

	return TSPERR(TSS_E_INTERNAL_ERROR);
}

TSS_RESULT
pcrs_sanity_check_selection(TSS_HCONTEXT tspContext,
			    struct tr_pcrs_obj *pcrs,
			    TPM_PCR_SELECTION *select)
{
	UINT16 num_pcrs, bytes_to_hold;

	if ((num_pcrs = get_num_pcrs(tspContext)) == 0)
		return TSPERR(TSS_E_INTERNAL_ERROR);

	bytes_to_hold = num_pcrs / 8;

	/* Is the current select object going to be interpretable by the TPM?
	 * If the select object is of a size greater than the one the TPM
	 * wants, just calculate the composite hash and let the TPM return an
	 * error code to the user.  If its less than the size of the one the
	 * TPM wants, add extra zero bytes until its the right size. */
	if (bytes_to_hold > select->sizeOfSelect) {
		if ((select->pcrSelect = realloc(select->pcrSelect, bytes_to_hold)) == NULL) {
			LogError("malloc of %hu bytes failed.", bytes_to_hold);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		/* set the newly allocated bytes to 0 */
		memset(&select->pcrSelect[select->sizeOfSelect], 0,
				bytes_to_hold - select->sizeOfSelect);
		select->sizeOfSelect = bytes_to_hold;

		/* realloc the pcr array as well */
		if ((pcrs->pcrs = realloc(pcrs->pcrs,
					  (bytes_to_hold * 8) * TPM_SHA1_160_HASH_LEN)) == NULL) {
			LogError("malloc of %d bytes failed.",
				 (bytes_to_hold * 8) * TPM_SHA1_160_HASH_LEN);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
	}

#ifdef TSS_DEBUG
	{
		int i;
		for (i = 0; i < select->sizeOfSelect * 8; i++) {
			if (select->pcrSelect[i/8] & (1 << (i % 8))) {
				LogDebug("PCR%d: Selected", i);
				LogBlobData(APPID, TPM_SHA1_160_HASH_LEN,
					    (unsigned char *)&pcrs->pcrs[i]);
			} else {
				LogDebug("PCR%d: Not Selected", i);
			}
		}
	}
#endif

	return TSS_SUCCESS;
}
