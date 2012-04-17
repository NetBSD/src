
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
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "trousers/tss.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tddl.h"
#include "req_mgr.h"
#include "tcslog.h"

static struct tpm_req_mgr *trm;

#ifdef TSS_DEBUG
#define TSS_TPM_DEBUG
#endif

TSS_RESULT
req_mgr_submit_req(BYTE *blob)
{
	TSS_RESULT result;
	BYTE loc_buf[TSS_TPM_TXBLOB_SIZE];
	UINT32 size = TSS_TPM_TXBLOB_SIZE;
	UINT32 retry = TSS_REQ_MGR_MAX_RETRIES;

	MUTEX_LOCK(trm->queue_lock);

#ifdef TSS_TPM_DEBUG
	LogBlobData("To TPM:", Decode_UINT32(&blob[2]), blob);
#endif

	do {
		result = Tddli_TransmitData(blob, Decode_UINT32(&blob[2]), loc_buf, &size);
	} while (!result && (Decode_UINT32(&loc_buf[6]) == TCPA_E_RETRY) && --retry);

	if (!result)
		memcpy(blob, loc_buf, Decode_UINT32(&loc_buf[2]));

#ifdef TSS_TPM_DEBUG
	LogBlobData("From TPM:", size, loc_buf);
#endif

	MUTEX_UNLOCK(trm->queue_lock);

	return result;
}

TSS_RESULT
req_mgr_init()
{
	if ((trm = calloc(1, sizeof(struct tpm_req_mgr))) == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(struct tpm_req_mgr));
		return TSS_E_OUTOFMEMORY;
	}

	MUTEX_INIT(trm->queue_lock);

	return Tddli_Open();
}

TSS_RESULT
req_mgr_final()
{
	free(trm);

	return Tddli_Close();
}

