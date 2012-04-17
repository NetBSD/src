
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
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcsps.h"
#include "tcslog.h"
#include "tddl.h"
#include "req_mgr.h"
#include "tcsd_wrap.h"
#include "tcsd.h"


TSS_RESULT
UnloadBlob_PCR_SELECTION(UINT64 *offset, BYTE *blob, TCPA_PCR_SELECTION *pcr)
{
	if (!pcr) {
		UINT16 size;

		UnloadBlob_UINT16(offset, &size, blob);

		if (size > 0)
			UnloadBlob(offset, size, blob, NULL);

		return TSS_SUCCESS;
	}

	UnloadBlob_UINT16(offset, &pcr->sizeOfSelect, blob);
	pcr->pcrSelect = malloc(pcr->sizeOfSelect);
        if (pcr->pcrSelect == NULL) {
		LogError("malloc of %hu bytes failed.", pcr->sizeOfSelect);
		pcr->sizeOfSelect = 0;
                return TCSERR(TSS_E_OUTOFMEMORY);
        }
	UnloadBlob(offset, pcr->sizeOfSelect, blob, pcr->pcrSelect);

	return TSS_SUCCESS;
}

void
LoadBlob_PCR_SELECTION(UINT64 *offset, BYTE * blob, TCPA_PCR_SELECTION pcr)
{
	LoadBlob_UINT16(offset, pcr.sizeOfSelect, blob);
	LoadBlob(offset, pcr.sizeOfSelect, blob, pcr.pcrSelect);
}

TSS_RESULT
UnloadBlob_PCR_COMPOSITE(UINT64 *offset, BYTE *blob, TCPA_PCR_COMPOSITE *out)
{
	TSS_RESULT rc;

	if (!out) {
		UINT32 size;

		if ((rc = UnloadBlob_PCR_SELECTION(offset, blob, NULL)))
			return rc;

		UnloadBlob_UINT32(offset, &size, blob);
		if (size > 0)
			UnloadBlob(offset, size, blob, NULL);

		return TSS_SUCCESS;
	}

	if ((rc = UnloadBlob_PCR_SELECTION(offset, blob, &out->select)))
		return rc;

	UnloadBlob_UINT32(offset, &out->valueSize, blob);
	out->pcrValue = malloc(out->valueSize);
        if (out->pcrValue == NULL) {
		LogError("malloc of %u bytes failed.", out->valueSize);
		out->valueSize = 0;
                return TCSERR(TSS_E_OUTOFMEMORY);
        }
	UnloadBlob(offset, out->valueSize, blob, (BYTE *) out->pcrValue);

	return TSS_SUCCESS;
}
