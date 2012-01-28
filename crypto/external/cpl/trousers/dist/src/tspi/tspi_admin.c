
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
#include <inttypes.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


TSS_RESULT
Tspi_TPM_SetStatus(TSS_HTPM hTPM,	/* in */
		   TSS_FLAG statusFlag,	/* in */
		   TSS_BOOL fTpmState)	/* in */
{
	TPM_AUTH auth, *pAuth;
	TSS_RESULT result;
	TCPA_DIGEST hashDigest;
	TSS_HCONTEXT tspContext;
	TSS_HPOLICY hPolicy;
	TSS_HPOLICY hOperatorPolicy;
	Trspi_HashCtx hashCtx;
	UINT32 tpmVersion;

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = obj_tpm_get_policy(hTPM, TSS_POLICY_USAGE, &hPolicy)))
		return result;

	switch (statusFlag) {
	case TSS_TPMSTATUS_DISABLEOWNERCLEAR:
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_DisableOwnerClear);
		if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
			return result;

		if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_DisableOwnerClear, hPolicy,
						      FALSE, &hashDigest, &auth)))
			return result;

		if ((result = TCS_API(tspContext)->DisableOwnerClear(tspContext, &auth)))
			return result;

		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_DisableOwnerClear);
		if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
			return result;

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &hashDigest, &auth)))
			return result;
		break;
	case TSS_TPMSTATUS_DISABLEFORCECLEAR:
		result = TCS_API(tspContext)->DisableForceClear(tspContext);
		break;
	case TSS_TPMSTATUS_DISABLED:
	case TSS_TPMSTATUS_OWNERSETDISABLE:
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_OwnerSetDisable);
		result |= Trspi_Hash_BOOL(&hashCtx, fTpmState);
		if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
			return result;

		if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_OwnerSetDisable, hPolicy,
						      FALSE, &hashDigest, &auth)))
			return result;

		if ((result = TCS_API(tspContext)->OwnerSetDisable(tspContext, fTpmState, &auth)))
			return result;

		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_OwnerSetDisable);
		if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
			return result;

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &hashDigest, &auth)))
			return result;
		break;
	case TSS_TPMSTATUS_PHYSICALDISABLE:
		if (fTpmState)
			result = TCS_API(tspContext)->PhysicalDisable(tspContext);
		else
			result = TCS_API(tspContext)->PhysicalEnable(tspContext);
		break;
	case TSS_TPMSTATUS_DEACTIVATED:
	case TSS_TPMSTATUS_PHYSICALSETDEACTIVATED:
		result = TCS_API(tspContext)->PhysicalSetDeactivated(tspContext, fTpmState);
		break;
	case TSS_TPMSTATUS_SETTEMPDEACTIVATED:
		if ((result = obj_context_get_tpm_version(tspContext, &tpmVersion)))
			return result;

		/* XXX Change 0,1,2 to #defines */
		switch (tpmVersion) {
		case 0:
		case 1:
			result = TCS_API(tspContext)->SetTempDeactivated(tspContext);
			break;
		case 2:
			if ((result = obj_tpm_get_policy(hTPM, TSS_POLICY_OPERATOR, &hOperatorPolicy)))
				return result;

			if (hOperatorPolicy != NULL_HPOLICY) {
				result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
				result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_SetTempDeactivated);
				if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
					return result;

				pAuth = &auth;
				if ((result = secret_PerformAuth_OIAP(hTPM,
								      TPM_ORD_SetTempDeactivated,
								      hOperatorPolicy, FALSE,
								      &hashDigest, pAuth)))
					return result;
			}
			else
				pAuth = NULL;

			if ((result = TCS_API(tspContext)->SetTempDeactivated2(tspContext, pAuth)))
				return result;

			if (pAuth) {
				result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
				result |= Trspi_Hash_UINT32(&hashCtx, result);
				result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_SetTempDeactivated);
				if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
					return result;

				if ((result = obj_policy_validate_auth_oiap(hOperatorPolicy,
									    &hashDigest,
									    pAuth)))
					return result;
			}
			break;
		default:
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		break;
	case TSS_TPMSTATUS_SETOWNERINSTALL:
		result = TCS_API(tspContext)->SetOwnerInstall(tspContext, fTpmState);
		break;
	case TSS_TPMSTATUS_DISABLEPUBEKREAD:
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_DisablePubekRead);
		if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
			return result;

		if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_DisablePubekRead, hPolicy,
						      FALSE, &hashDigest, &auth)))
			return result;

		if ((result = TCS_API(tspContext)->DisablePubekRead(tspContext, &auth)))
			return result;

		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_DisablePubekRead);
		if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
			return result;

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &hashDigest, &auth)))
			return result;
		break;
	case TSS_TPMSTATUS_ALLOWMAINTENANCE:
		/* Allow maintenance cannot be set to TRUE in the TPM */
		if (fTpmState)
			return TSPERR(TSS_E_BAD_PARAMETER);

		/* The path to setting allow maintenance to FALSE is through
		 * KillMaintenanceFeature */
		return Tspi_TPM_KillMaintenanceFeature(hTPM);
		break;
#ifdef TSS_BUILD_TSS12
	case TSS_TPMSTATUS_DISABLEPUBSRKREAD:
		/* The logic of setting a 'disable' flag is reversed in the TPM, where setting this
		 * flag to TRUE will enable the SRK read, while FALSE disables it. So we need to
		 * flip the bool here. Sigh... */
		fTpmState = fTpmState ? FALSE : TRUE;

		result = TSP_SetCapability(tspContext, hTPM, hPolicy, TPM_SET_PERM_FLAGS,
					   TPM_PF_READSRKPUB, fTpmState);
		break;
	case TSS_TPMSTATUS_RESETLOCK:
		/* ignoring the bool here */
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ResetLockValue);
		if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
			return result;

		if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_ResetLockValue, hPolicy,
						      FALSE, &hashDigest, &auth)))
			return result;

		if ((result = TCS_API(tspContext)->ResetLockValue(tspContext, &auth)))
			return result;

		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ResetLockValue);
		if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
			return result;

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &hashDigest, &auth)))
			return result;
		break;
#endif
#ifndef TSS_SPEC_COMPLIANCE
	case TSS_TPMSTATUS_PHYSPRES_LIFETIMELOCK:
		/* set the lifetime lock bit */
		result = TCS_API(tspContext)->PhysicalPresence(tspContext,
							       TPM_PHYSICAL_PRESENCE_LIFETIME_LOCK);
		break;
	case TSS_TPMSTATUS_PHYSPRES_HWENABLE:
		/* set the HW enable bit */
		result = TCS_API(tspContext)->PhysicalPresence(tspContext,
							       TPM_PHYSICAL_PRESENCE_HW_ENABLE);
		break;
	case TSS_TPMSTATUS_PHYSPRES_CMDENABLE:
		/* set the command enable bit */
		result = TCS_API(tspContext)->PhysicalPresence(tspContext,
							       TPM_PHYSICAL_PRESENCE_CMD_ENABLE);
		break;
	case TSS_TPMSTATUS_PHYSPRES_LOCK:
		/* set the physical presence lock bit */
		result = TCS_API(tspContext)->PhysicalPresence(tspContext,
							       TPM_PHYSICAL_PRESENCE_LOCK);
		break;
	case TSS_TPMSTATUS_PHYSPRESENCE:
		/* set the physical presence state */
		result = TCS_API(tspContext)->PhysicalPresence(tspContext, (fTpmState ?
							       TPM_PHYSICAL_PRESENCE_PRESENT :
							       TPM_PHYSICAL_PRESENCE_NOTPRESENT));
		break;
#endif
	default:
		return TSPERR(TSS_E_BAD_PARAMETER);
		break;
	}

	return result;
}

TSS_RESULT
Tspi_TPM_GetStatus(TSS_HTPM hTPM,		/* in */
		   TSS_FLAG statusFlag,		/* in */
		   TSS_BOOL * pfTpmState)	/* out */
{
	TSS_HCONTEXT tspContext;
	TSS_RESULT result;
	UINT32 nonVolFlags;
	UINT32 volFlags;

	if (pfTpmState == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = get_tpm_flags(tspContext, hTPM, &volFlags, &nonVolFlags)))
		return result;

	switch (statusFlag) {
	case TSS_TPMSTATUS_DISABLEOWNERCLEAR:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_DISABLEOWNERCLEAR_BIT);
		break;
	case TSS_TPMSTATUS_DISABLEFORCECLEAR:
		*pfTpmState = BOOL(volFlags & TSS_TPM_SF_DISABLEFORCECLEAR_BIT);
		break;
	case TSS_TPMSTATUS_DISABLED:
	case TSS_TPMSTATUS_OWNERSETDISABLE:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_DISABLE_BIT);
		break;
	case TSS_TPMSTATUS_DEACTIVATED:
	case TSS_TPMSTATUS_PHYSICALSETDEACTIVATED:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_DEACTIVATED_BIT);
		break;
	case TSS_TPMSTATUS_SETTEMPDEACTIVATED:
		*pfTpmState = BOOL(volFlags & TSS_TPM_SF_DEACTIVATED_BIT);
		break;
	case TSS_TPMSTATUS_SETOWNERINSTALL:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_OWNERSHIP_BIT);
		break;
	case TSS_TPMSTATUS_DISABLEPUBEKREAD:
		*pfTpmState = INVBOOL(nonVolFlags & TSS_TPM_PF_READPUBEK_BIT);
		break;
	case TSS_TPMSTATUS_ALLOWMAINTENANCE:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_ALLOWMAINTENANCE_BIT);
		break;
	case TSS_TPMSTATUS_MAINTENANCEUSED:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_MAINTENANCEDONE_BIT);
		break;
	case TSS_TPMSTATUS_PHYSPRES_LIFETIMELOCK:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_PHYSICALPRESENCELIFETIMELOCK_BIT);
		break;
	case TSS_TPMSTATUS_PHYSPRES_HWENABLE:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_PHYSICALPRESENCEHWENABLE_BIT);
		break;
	case TSS_TPMSTATUS_PHYSPRES_CMDENABLE:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_PHYSICALPRESENCECMDENABLE_BIT);
		break;
	case TSS_TPMSTATUS_CEKP_USED:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_CEKPUSED_BIT);
		break;
	case TSS_TPMSTATUS_PHYSPRESENCE:
		*pfTpmState = BOOL(volFlags & TSS_TPM_SF_PHYSICALPRESENCE_BIT);
		break;
	case TSS_TPMSTATUS_PHYSPRES_LOCK:
		*pfTpmState = BOOL(volFlags & TSS_TPM_SF_PHYSICALPRESENCELOCK_BIT);
		break;
	case TSS_TPMSTATUS_TPMPOST:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_TPMPOST_BIT);
		break;
	case TSS_TPMSTATUS_TPMPOSTLOCK:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_TPMPOSTLOCK_BIT);
		break;
	case TSS_TPMSTATUS_FIPS:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_FIPS_BIT);
		break;
	case TSS_TPMSTATUS_ENABLE_REVOKEEK:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_ENABLEREVOKEEK_BIT);
		break;
	case TSS_TPMSTATUS_TPM_ESTABLISHED:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_RESETESTABLISHMENTBIT_BIT);
		break;
	case TSS_TPMSTATUS_NV_LOCK:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_NV_LOCKED_BIT);
		break;
	case TSS_TPMSTATUS_POSTINITIALISE:
		/* There is no way to query the TPM for this flag. */
		result = TSPERR(TSS_E_NOTIMPL);
		break;
#ifdef TSS_BUILD_TSS12
	case TSS_TPMSTATUS_DISABLEPUBSRKREAD:
		*pfTpmState = INVBOOL(nonVolFlags & TSS_TPM_PF_READSRKPUB_BIT);
		break;
	case TSS_TPMSTATUS_OPERATORINSTALLED:
		*pfTpmState = BOOL(nonVolFlags & TSS_TPM_PF_OPERATOR_BIT);
		break;
#endif
	default:
		return TSPERR(TSS_E_BAD_PARAMETER);
		break;
	}

	return TSS_SUCCESS;
}
