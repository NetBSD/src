
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
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcslog.h"


TSS_RESULT
UnloadBlob_STORED_DATA(UINT64 *offset, BYTE *blob, TCPA_STORED_DATA *data)
{
	if (!data) {
		UINT32 size;

		UnloadBlob_VERSION(offset, blob, NULL);

		UnloadBlob_UINT32(offset, &size, blob);

		if (size > 0)
			UnloadBlob(offset, size, blob, NULL);

		UnloadBlob_UINT32(offset, &size, blob);

		if (size > 0)
			UnloadBlob(offset, size, blob, NULL);

		return TSS_SUCCESS;
	}

	UnloadBlob_VERSION(offset, blob, (TPM_VERSION *)&data->ver);

	UnloadBlob_UINT32(offset, &data->sealInfoSize, blob);

	if (data->sealInfoSize > 0) {
		data->sealInfo = (BYTE *)calloc(1, data->sealInfoSize);
		if (data->sealInfo == NULL) {
			LogError("malloc of %u bytes failed.", data->sealInfoSize);
			data->sealInfoSize = 0;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(offset, data->sealInfoSize, blob, data->sealInfo);
	} else {
		data->sealInfo = NULL;
	}

	UnloadBlob_UINT32(offset, &data->encDataSize, blob);

	if (data->encDataSize > 0) {
		data->encData = (BYTE *)calloc(1, data->encDataSize);
		if (data->encData == NULL) {
			LogError("malloc of %u bytes failed.", data->encDataSize);
			data->encDataSize = 0;
			free(data->sealInfo);
			data->sealInfo = NULL;
			data->sealInfoSize = 0;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(offset, data->encDataSize, blob, data->encData);
	} else {
		data->encData = NULL;
	}

	return TSS_SUCCESS;
}
