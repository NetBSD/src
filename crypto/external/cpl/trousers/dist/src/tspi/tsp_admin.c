
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2007
 *
 */


#include <stdlib.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_GetCapabilityOwner(TSS_HCONTEXT tspContext,      /* in */
			     TPM_AUTH * pOwnerAuth,        /* in/out */
			     TCPA_VERSION * pVersion,      /* out */
			     UINT32 * pNonVolatileFlags,   /* out */
			     UINT32 * pVolatileFlags)      /* out */
{
	UINT64 offset;
	TSS_RESULT result;
	UINT32 handlesLen = 0, decLen;
	BYTE *dec;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_GetCapabilityOwner, 0, NULL,
						    NULL, &handlesLen, NULL, pOwnerAuth, NULL,
						    &decLen, &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_TCPA_VERSION(&offset, dec, pVersion);
	Trspi_UnloadBlob_UINT32(&offset, pNonVolatileFlags, dec);
	Trspi_UnloadBlob_UINT32(&offset, pVolatileFlags, dec);

	free(dec);

	return result;
}

TSS_RESULT
Transport_SetOwnerInstall(TSS_HCONTEXT tspContext, /* in */
			  TSS_BOOL state)  /* in */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	result = obj_context_transport_execute(tspContext, TPM_ORD_SetOwnerInstall,
					       sizeof(TSS_BOOL), (BYTE *)&state, NULL, &handlesLen,
					       NULL, NULL, NULL, NULL, NULL);

	return result;
}

TSS_RESULT
Transport_DisableOwnerClear(TSS_HCONTEXT tspContext,       /* in */
			    TPM_AUTH * ownerAuth)  /* in, out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	result = obj_context_transport_execute(tspContext, TPM_ORD_DisableOwnerClear, 0, NULL, NULL,
					       &handlesLen, NULL, ownerAuth, NULL, NULL, NULL);

	return result;
}

TSS_RESULT
Transport_DisableForceClear(TSS_HCONTEXT tspContext)       /* in */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	result = obj_context_transport_execute(tspContext, TPM_ORD_DisableForceClear, 0, NULL, NULL,
					       &handlesLen, NULL, NULL, NULL, NULL, NULL);

	return result;
}

TSS_RESULT
Transport_OwnerSetDisable(TSS_HCONTEXT tspContext, /* in */
			  TSS_BOOL disableState,   /* in */
			  TPM_AUTH * ownerAuth)    /* in, out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	result = obj_context_transport_execute(tspContext, TPM_ORD_OwnerSetDisable,
					       sizeof(TSS_BOOL), (BYTE *)&disableState, NULL,
					       &handlesLen, NULL, ownerAuth, NULL, NULL, NULL);

	return result;
}

TSS_RESULT
Transport_PhysicalDisable(TSS_HCONTEXT tspContext) /* in */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	result = obj_context_transport_execute(tspContext, TPM_ORD_PhysicalDisable, 0, NULL, NULL,
					       &handlesLen, NULL, NULL, NULL, NULL, NULL);

	return result;
}

TSS_RESULT
Transport_PhysicalEnable(TSS_HCONTEXT tspContext)  /* in */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	result = obj_context_transport_execute(tspContext, TPM_ORD_PhysicalEnable, 0, NULL, NULL,
					       &handlesLen, NULL, NULL, NULL, NULL, NULL);

	return result;
}

TSS_RESULT
Transport_PhysicalSetDeactivated(TSS_HCONTEXT tspContext,  /* in */
				 TSS_BOOL state)   /* in */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	result = obj_context_transport_execute(tspContext, TPM_ORD_PhysicalSetDeactivated,
					       sizeof(TSS_BOOL), (BYTE *)&state, NULL, &handlesLen,
					       NULL, NULL, NULL, NULL, NULL);

	return result;
}

TSS_RESULT
Transport_SetTempDeactivated(TSS_HCONTEXT tspContext)      /* in */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	result = obj_context_transport_execute(tspContext, TPM_ORD_SetTempDeactivated, 0, NULL,
					       NULL, &handlesLen, NULL, NULL, NULL, NULL, NULL);

	return result;
}

TSS_RESULT
Transport_SetTempDeactivated2(TSS_HCONTEXT tspContext,     /* in */
			      TPM_AUTH *operatorAuth)      /* in, out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	result = obj_context_transport_execute(tspContext, TPM_ORD_SetTempDeactivated, 0, NULL,
					       NULL, &handlesLen, NULL, operatorAuth, NULL, NULL,
					       NULL);

	return result;
}

TSS_RESULT
Transport_DisablePubekRead(TSS_HCONTEXT tspContext,        /* in */
			   TPM_AUTH * ownerAuth)   /* in, out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	result = obj_context_transport_execute(tspContext, TPM_ORD_DisablePubekRead, 0, NULL, NULL,
					       &handlesLen, NULL, ownerAuth, NULL, NULL, NULL);

	return result;
}

TSS_RESULT
Transport_ResetLockValue(TSS_HCONTEXT tspContext,  /* in */
			 TPM_AUTH * ownerAuth)     /* in, out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	result = obj_context_transport_execute(tspContext, TPM_ORD_ResetLockValue, 0, NULL, NULL,
					       &handlesLen, NULL, ownerAuth, NULL, NULL, NULL);

	return result;
}

TSS_RESULT
Transport_PhysicalPresence(TSS_HCONTEXT tspContext,        /* in */
			   TCPA_PHYSICAL_PRESENCE fPhysicalPresence)       /* in */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	result = obj_context_transport_execute(tspContext, TSC_ORD_PhysicalPresence,
					       sizeof(TCPA_PHYSICAL_PRESENCE),
					       (BYTE *)&fPhysicalPresence, NULL, &handlesLen, NULL,
					       NULL, NULL, NULL, NULL);

	return result;
}

TSS_RESULT
Transport_FlushSpecific(TSS_HCONTEXT tspContext, /* in */
			TCS_HANDLE hResHandle, /* in */
			TPM_RESOURCE_TYPE resourceType) /* in */
{
	UINT64 offset;
	TSS_RESULT result;
	UINT32 handlesLen = 1;
	TCS_HANDLE *handles, handle;
	BYTE data[sizeof(UINT32)];

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	handle = hResHandle;
	handles = &handle;

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, resourceType, data);

	result = obj_context_transport_execute(tspContext, TPM_ORD_FlushSpecific, sizeof(data),
					       data, NULL, &handlesLen, &handles, NULL, NULL, NULL,
					       NULL);

	return result;
}
#endif

