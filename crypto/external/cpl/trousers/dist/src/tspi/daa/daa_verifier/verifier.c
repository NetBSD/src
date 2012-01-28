
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bi.h"
#include "daa_parameter.h"
#include "trousers/tss.h"
#include "spi_internal_types.h"
#include "spi_utils.h"
#include <trousers/trousers.h>
#include <obj.h>
#include "tsplog.h"
#include "tss/tcs.h"
#include "platform.h"

#include "verifier.h"

TSPICALL Tspi_DAA_VerifyInit_internal
(
	TSS_HDAA	hDAA,	// in
	UINT32*	nonceVerifierLength,	// out
	BYTE**	nonceVerifier,	// out
	UINT32	baseNameLength,	// out
	BYTE **	baseName	// out
) {
	TSS_RESULT result = TSS_SUCCESS;
	TCS_CONTEXT_HANDLE tcsContext;
	bi_ptr nounce = NULL;

	//TODO how to setup the baseName & baseNameLength
	if( (result = obj_daa_get_tsp_context( hDAA, &tcsContext)) != TSS_SUCCESS)
		goto close;
	*nonceVerifierLength = DAA_PARAM_LENGTH_MESSAGE_DIGEST;
	*nonceVerifier = calloc_tspi( tcsContext, DAA_PARAM_LENGTH_MESSAGE_DIGEST);
	if (*nonceVerifier == NULL) {
		LogError("malloc of %d bytes failed", DAA_PARAM_LENGTH_MESSAGE_DIGEST);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	nounce = bi_new_ptr();
	bi_urandom( nounce, DAA_PARAM_LENGTH_MESSAGE_DIGEST * 8);
	bi_2_byte_array( *nonceVerifier, DAA_PARAM_LENGTH_MESSAGE_DIGEST, nounce);
close:
	FREE_BI( nounce);
	return result;
}
