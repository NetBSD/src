
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
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "obj.h"
#include "tsplog.h"


TSS_RESULT
Tspi_TPM_SetOperatorAuth(TSS_HTPM    hTpm,		/* in */
			 TSS_HPOLICY hOperatorPolicy)	/* in */
{
	TSS_HCONTEXT tspContext;
	UINT32 type;
	TCPA_SECRET operatorAuth;
	TSS_RESULT result = TSS_SUCCESS;

	if ((result = obj_tpm_get_tsp_context(hTpm, &tspContext)))
		return result;

	if ((result = obj_policy_get_type(hOperatorPolicy, &type)))
		return result;

	if (type != TSS_POLICY_OPERATOR)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_policy_get_secret(hOperatorPolicy, TR_SECRET_CTX_NEW, &operatorAuth)))
		return result;

	if ((result = TCS_API(tspContext)->SetOperatorAuth(tspContext, &operatorAuth)))
		return result;

	if ((result = obj_tpm_set_policy(hTpm, hOperatorPolicy)))
		return result;

	return result;
}
