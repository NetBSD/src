
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "tcs_tsp.h"
#include "tspps.h"
#include "hosttable.h"
#include "tcsd_wrap.h"
#include "tcsd.h"
#include "obj.h"


TSS_RESULT
Tspi_Context_Create(TSS_HCONTEXT * phContext)	/* out */
{
	if (phContext == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	return obj_context_add(phContext);
}

TSS_RESULT
Tspi_Context_Close(TSS_HCONTEXT tspContext)	/* in */
{
	if (!obj_is_context(tspContext))
		return TSPERR(TSS_E_INVALID_HANDLE);

	obj_context_close(tspContext);

	/* Have the TCS do its thing */
	RPC_CloseContext(tspContext);

	/* Note: Memory that was returned to the app that was alloc'd by this
	 * context isn't free'd here.  Any memory that the app doesn't explicitly
	 * free is left for it to free itself. */

	/* Destroy all objects */
	obj_close_context(tspContext);

	Tspi_Context_FreeMemory(tspContext, NULL);

	/* close the ps file */
	PS_close();

	/* We're not a connected context, so just exit */
	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_Context_Connect(TSS_HCONTEXT tspContext,	/* in */
		     TSS_UNICODE *wszDestination)	/* in */
{
	TSS_RESULT result;
	BYTE *machine_name = NULL;
	TSS_HOBJECT hTpm;
	UINT32 string_len = 0;


	if (wszDestination == NULL) {
		if ((result = obj_context_get_machine_name(tspContext,
							   &string_len,
							   &machine_name)))
			return result;

		if ((result = RPC_OpenContext(tspContext, machine_name,
					      CONNECTION_TYPE_TCP_PERSISTANT)))
			return result;
	} else {
		if ((machine_name =
		    Trspi_UNICODE_To_Native((BYTE *)wszDestination, NULL)) == NULL) {
			LogError("Error converting hostname to UTF-8");
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}

		if ((result = RPC_OpenContext(tspContext, machine_name,
					      CONNECTION_TYPE_TCP_PERSISTANT)))
			return result;

		if ((result = obj_context_set_machine_name(tspContext, machine_name,
						strlen((char *)machine_name)+1)))
			return result;
	}

        if ((obj_tpm_add(tspContext, &hTpm)))
                return TSPERR(TSS_E_INTERNAL_ERROR);

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_Context_FreeMemory(TSS_HCONTEXT tspContext,	/* in */
			BYTE * rgbMemory)		/* in */
{
	if (!obj_is_context(tspContext))
		return TSPERR(TSS_E_INVALID_HANDLE);

	return free_tspi(tspContext, rgbMemory);
}

TSS_RESULT
Tspi_Context_GetDefaultPolicy(TSS_HCONTEXT tspContext,	/* in */
			      TSS_HPOLICY * phPolicy)	/* out */
{
	if (phPolicy == NULL )
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (!obj_is_context(tspContext))
		return TSPERR(TSS_E_INVALID_HANDLE);

	return obj_context_get_policy(tspContext, TSS_POLICY_USAGE, phPolicy);
}

TSS_RESULT
Tspi_Context_CreateObject(TSS_HCONTEXT tspContext,	/* in */
			  TSS_FLAG objectType,		/* in */
			  TSS_FLAG initFlags,		/* in */
			  TSS_HOBJECT * phObject)	/* out */
{
	TSS_RESULT result;

	if (phObject == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (!obj_is_context(tspContext))
		return TSPERR(TSS_E_INVALID_HANDLE);

	switch (objectType) {
	case TSS_OBJECT_TYPE_POLICY:
		switch (initFlags) {
#ifdef TSS_BUILD_TSS12
			case TSS_POLICY_OPERATOR:
				/* fall through */
#endif
			case TSS_POLICY_MIGRATION:
				/* fall through */
			case TSS_POLICY_USAGE:
				break;
			default:
				return TSPERR(TSS_E_INVALID_OBJECT_INITFLAG);
		}

		result = obj_policy_add(tspContext, initFlags, phObject);
		break;
#ifdef TSS_BUILD_RSAKEY_LIST
	case TSS_OBJECT_TYPE_RSAKEY:
		/* If other flags are set that disagree with the SRK, this will
		 * help catch that conflict in the later steps */
		if (initFlags & TSS_KEY_TSP_SRK) {
			initFlags |= (TSS_KEY_TYPE_STORAGE | TSS_KEY_NOT_MIGRATABLE |
				      TSS_KEY_NON_VOLATILE | TSS_KEY_SIZE_2048);
		}

		/* Set default key flags */

		/* Default key size = 2k */
		if ((initFlags & TSS_KEY_SIZE_MASK) == 0)
			initFlags |= TSS_KEY_SIZE_2048;

		/* Default key type = storage */
		if ((initFlags & TSS_KEY_TYPE_MASK) == 0)
			initFlags |= TSS_KEY_TYPE_STORAGE;

		/* Check the key flags */
		switch (initFlags & TSS_KEY_SIZE_MASK) {
			case TSS_KEY_SIZE_512:
				/* fall through */
			case TSS_KEY_SIZE_1024:
				/* fall through */
			case TSS_KEY_SIZE_2048:
				/* fall through */
			case TSS_KEY_SIZE_4096:
				/* fall through */
			case TSS_KEY_SIZE_8192:
				/* fall through */
			case TSS_KEY_SIZE_16384:
				break;
			default:
				return TSPERR(TSS_E_INVALID_OBJECT_INITFLAG);
		}

		switch (initFlags & TSS_KEY_TYPE_MASK) {
			case TSS_KEY_TYPE_STORAGE:
				/* fall through */
			case TSS_KEY_TYPE_SIGNING:
				/* fall through */
			case TSS_KEY_TYPE_BIND:
				/* fall through */
			case TSS_KEY_TYPE_AUTHCHANGE:
				/* fall through */
			case TSS_KEY_TYPE_LEGACY:
				/* fall through */
			case TSS_KEY_TYPE_IDENTITY:
				break;
			default:
				return TSPERR(TSS_E_INVALID_OBJECT_INITFLAG);
		}

		result = obj_rsakey_add(tspContext, initFlags, phObject);
		break;
#endif
#ifdef TSS_BUILD_ENCDATA_LIST
	case TSS_OBJECT_TYPE_ENCDATA:
		switch (initFlags & TSS_ENCDATA_TYPE_MASK) {
			case TSS_ENCDATA_LEGACY:
				/* fall through */
			case TSS_ENCDATA_SEAL:
				/* fall through */
			case TSS_ENCDATA_BIND:
				break;
			default:
				return TSPERR(TSS_E_INVALID_OBJECT_INITFLAG);
		}

		result = obj_encdata_add(tspContext, (initFlags & TSS_ENCDATA_TYPE_MASK), phObject);
		break;
#endif
#ifdef TSS_BUILD_PCRS_LIST
	case TSS_OBJECT_TYPE_PCRS:
		switch (initFlags) {
			case TSS_PCRS_STRUCT_DEFAULT:
				/* fall through */
			case TSS_PCRS_STRUCT_INFO:
				/* fall through */
			case TSS_PCRS_STRUCT_INFO_LONG:
				/* fall through */
			case TSS_PCRS_STRUCT_INFO_SHORT:
				/* fall through */
				break;
			default:
				return TSPERR(TSS_E_INVALID_OBJECT_INITFLAG);
		}

		result = obj_pcrs_add(tspContext, initFlags, phObject);
		break;
#endif
#ifdef TSS_BUILD_HASH_LIST
	case TSS_OBJECT_TYPE_HASH:
		switch (initFlags) {
			case TSS_HASH_DEFAULT:
				/* fall through */
			case TSS_HASH_SHA1:
				/* fall through */
			case TSS_HASH_OTHER:
				break;
			default:
				return TSPERR(TSS_E_INVALID_OBJECT_INITFLAG);
		}

		result = obj_hash_add(tspContext, initFlags, phObject);
		break;
#endif
#ifdef TSS_BUILD_DAA
	//case TSS_OBJECT_TYPE_DAA_CREDENTIAL:
	case TSS_OBJECT_TYPE_DAA_CERTIFICATE:
		if (initFlags & ~(0UL))
			return TSPERR(TSS_E_INVALID_OBJECT_INITFLAG);

		result = obj_daacred_add(tspContext, phObject);
		break;
	case TSS_OBJECT_TYPE_DAA_ISSUER_KEY:
		if (initFlags & ~(0UL))
			return TSPERR(TSS_E_INVALID_OBJECT_INITFLAG);

		result = obj_daaissuerkey_add(tspContext, phObject);
		break;
	case TSS_OBJECT_TYPE_DAA_ARA_KEY:
		if (initFlags & ~(0UL))
			return TSPERR(TSS_E_INVALID_OBJECT_INITFLAG);

		result = obj_daaarakey_add(tspContext, phObject);
		break;
#endif
#ifdef TSS_BUILD_NV
	case TSS_OBJECT_TYPE_NV:
		/* There are no valid flags for a NV object */
		if (initFlags & ~(0UL))
			return TSPERR(TSS_E_INVALID_OBJECT_INITFLAG);

		result = obj_nvstore_add(tspContext, phObject);
		break;
#endif
#ifdef TSS_BUILD_DELEGATION
	case TSS_OBJECT_TYPE_DELFAMILY:
		/* There are no valid flags for a DELFAMILY object */
		if (initFlags & ~(0UL))
			return TSPERR(TSS_E_INVALID_OBJECT_INITFLAG);

		result = obj_delfamily_add(tspContext, phObject);
		break;
#endif
#ifdef TSS_BUILD_CMK
	case TSS_OBJECT_TYPE_MIGDATA:
		/* There are no valid flags for a MIGDATA object */
		if (initFlags & ~(0UL))
			return TSPERR(TSS_E_INVALID_OBJECT_INITFLAG);
	
		result = obj_migdata_add(tspContext, phObject);
		break;
#endif
	default:
		LogDebug("Invalid Object type");
		return TSPERR(TSS_E_INVALID_OBJECT_TYPE);
		break;
	}

	return result;
}

TSS_RESULT
Tspi_Context_CloseObject(TSS_HCONTEXT tspContext,	/* in */
			 TSS_HOBJECT hObject)		/* in */
{
	TSS_RESULT result;

	if (!obj_is_context(tspContext))
		return TSPERR(TSS_E_INVALID_HANDLE);

	if (obj_is_pcrs(hObject)) {
#ifdef TSS_BUILD_PCRS_LIST
		result = obj_pcrs_remove(hObject, tspContext);
#endif
	} else if (obj_is_encdata(hObject)) {
#ifdef TSS_BUILD_ENCDATA_LIST
		result = obj_encdata_remove(hObject, tspContext);
#endif
	} else if (obj_is_hash(hObject)) {
#ifdef TSS_BUILD_HASH_LIST
		result = obj_hash_remove(hObject, tspContext);
#endif
	} else if (obj_is_rsakey(hObject)) {
#ifdef TSS_BUILD_RSAKEY_LIST
		result = obj_rsakey_remove(hObject, tspContext);
#endif
	} else if (obj_is_policy(hObject)) {
		result = obj_policy_remove(hObject, tspContext);
	} else if (obj_is_delfamily(hObject)) {
#ifdef TSS_BUILD_DELEGATION
		result = obj_delfamily_remove(hObject, tspContext);
#endif
	} else if (obj_is_migdata(hObject)) {
#ifdef TSS_BUILD_CMK
		result = obj_migdata_remove(hObject, tspContext);
#endif
	} else {
		result = TSPERR(TSS_E_INVALID_HANDLE);
	}

	return result;
}

TSS_RESULT
Tspi_Context_GetTpmObject(TSS_HCONTEXT tspContext,	/* in */
			  TSS_HTPM * phTPM)		/* out */
{
	if (phTPM == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (!obj_is_context(tspContext))
		return TSPERR(TSS_E_INVALID_HANDLE);

	return obj_tpm_get(tspContext, phTPM);
}

