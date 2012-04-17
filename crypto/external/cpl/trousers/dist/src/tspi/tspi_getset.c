
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
#include <time.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"
#include "tsp_audit.h"


TSS_RESULT
Tspi_SetAttribUint32(TSS_HOBJECT hObject,	/* in */
		     TSS_FLAG attribFlag,	/* in */
		     TSS_FLAG subFlag,		/* in */
		     UINT32 ulAttrib)		/* in */
{
	TSS_RESULT result;

	if (obj_is_rsakey(hObject)) {
#ifdef TSS_BUILD_RSAKEY_LIST
		if (attribFlag == TSS_TSPATTRIB_KEY_REGISTER) {
			if (subFlag)
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);

			if (ulAttrib == TSS_TSPATTRIB_KEYREGISTER_USER)
				result = obj_rsakey_set_pstype(hObject, TSS_PS_TYPE_USER);
			else if (ulAttrib == TSS_TSPATTRIB_KEYREGISTER_SYSTEM)
				result = obj_rsakey_set_pstype(hObject, TSS_PS_TYPE_SYSTEM);
			else if (ulAttrib == TSS_TSPATTRIB_KEYREGISTER_NO)
				result = obj_rsakey_set_pstype(hObject, TSS_PS_TYPE_NO);
			else
				return TSPERR(TSS_E_INVALID_ATTRIB_DATA);
		} else if (attribFlag == TSS_TSPATTRIB_KEY_INFO) {
			switch (subFlag) {
				case TSS_TSPATTRIB_KEYINFO_USAGE:
					result = obj_rsakey_set_usage(hObject, ulAttrib);
					break;
				case TSS_TSPATTRIB_KEYINFO_MIGRATABLE:
					if (ulAttrib != TRUE && ulAttrib != FALSE)
						return TSPERR(TSS_E_INVALID_ATTRIB_DATA);

					result = obj_rsakey_set_migratable(hObject, ulAttrib);
					break;
				case TSS_TSPATTRIB_KEYINFO_REDIRECTED:
					if (ulAttrib != TRUE && ulAttrib != FALSE)
						return TSPERR(TSS_E_INVALID_ATTRIB_DATA);

					result = obj_rsakey_set_redirected(hObject, ulAttrib);
					break;
				case TSS_TSPATTRIB_KEYINFO_VOLATILE:
					if (ulAttrib != TRUE && ulAttrib != FALSE)
						return TSPERR(TSS_E_INVALID_ATTRIB_DATA);

					result = obj_rsakey_set_volatile(hObject, ulAttrib);
					break;
				case TSS_TSPATTRIB_KEYINFO_AUTHUSAGE:
					/* fall through */
				case TSS_TSPATTRIB_KEYINFO_AUTHDATAUSAGE:
					if (ulAttrib != TRUE && ulAttrib != FALSE)
						return TSPERR(TSS_E_INVALID_ATTRIB_DATA);

					result = obj_rsakey_set_authdata_usage(hObject, ulAttrib);
					break;
				case TSS_TSPATTRIB_KEYINFO_ALGORITHM:
					result = obj_rsakey_set_alg(hObject, ulAttrib);
					break;
				case TSS_TSPATTRIB_KEYINFO_ENCSCHEME:
					if (ulAttrib != TSS_ES_NONE &&
					    ulAttrib != TSS_ES_RSAESPKCSV15 &&
					    ulAttrib != TSS_ES_RSAESOAEP_SHA1_MGF1)
						return TSPERR(TSS_E_INVALID_ATTRIB_DATA);

					result = obj_rsakey_set_es(hObject, ulAttrib);
					break;
				case TSS_TSPATTRIB_KEYINFO_SIGSCHEME:
					if (ulAttrib != TSS_SS_NONE &&
					    ulAttrib != TSS_SS_RSASSAPKCS1V15_SHA1 &&
					    ulAttrib != TSS_SS_RSASSAPKCS1V15_DER  &&
					    ulAttrib !=	TSS_SS_RSASSAPKCS1V15_INFO)
						return TSPERR(TSS_E_INVALID_ATTRIB_DATA);

					result = obj_rsakey_set_ss(hObject, ulAttrib);
					break;
				case TSS_TSPATTRIB_KEYINFO_KEYFLAGS:
					result = obj_rsakey_set_flags(hObject, ulAttrib);
					break;
				case TSS_TSPATTRIB_KEYINFO_SIZE:
					result = obj_rsakey_set_size(hObject, ulAttrib);
					break;
				default:
					return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
		} else if (attribFlag == TSS_TSPATTRIB_RSAKEY_INFO) {
			if (subFlag == TSS_TSPATTRIB_KEYINFO_RSA_PRIMES) {
				result = obj_rsakey_set_num_primes(hObject, ulAttrib);
			} else
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
		} else
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
#endif
#ifdef TSS_BUILD_NV
	} else if (obj_is_nvstore(hObject)) {
		switch (attribFlag) {
			case TSS_TSPATTRIB_NV_INDEX:
				if ((result = obj_nvstore_set_index(hObject, ulAttrib)))
					return result;
				break;
			case TSS_TSPATTRIB_NV_DATASIZE:
				if ((result = obj_nvstore_set_datasize(hObject, ulAttrib)))
					return result;
				break;
			case TSS_TSPATTRIB_NV_PERMISSIONS:
				if ((result = obj_nvstore_set_permission(hObject, ulAttrib)))
					return result;
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
				break;
		}
#endif
	} else if (obj_is_policy(hObject)) {
		switch (attribFlag) {
			case TSS_TSPATTRIB_POLICY_CALLBACK_HMAC:
			case TSS_TSPATTRIB_POLICY_CALLBACK_XOR_ENC:
			case TSS_TSPATTRIB_POLICY_CALLBACK_TAKEOWNERSHIP:
			case TSS_TSPATTRIB_POLICY_CALLBACK_CHANGEAUTHASYM:
				result = obj_policy_set_cb11(hObject, attribFlag,
							     subFlag, ulAttrib);
				break;
			case TSS_TSPATTRIB_POLICY_SECRET_LIFETIME:
				if (subFlag == TSS_TSPATTRIB_POLICYSECRET_LIFETIME_ALWAYS ||
				    subFlag == TSS_TSPATTRIB_POLICYSECRET_LIFETIME_COUNTER ||
				    subFlag == TSS_TSPATTRIB_POLICYSECRET_LIFETIME_TIMER) {
					result = obj_policy_set_lifetime(hObject, subFlag,
									 ulAttrib);
				} else {
					result = TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
				}
				break;
			case TSS_TSPATTRIB_SECRET_HASH_MODE:
				result = obj_policy_set_hash_mode(hObject, ulAttrib);
				break;
#ifdef TSS_BUILD_DELEGATION
			case TSS_TSPATTRIB_POLICY_DELEGATION_INFO:
				switch (subFlag) {
				case TSS_TSPATTRIB_POLDEL_TYPE:
					switch (ulAttrib) {
					case TSS_DELEGATIONTYPE_NONE:
					case TSS_DELEGATIONTYPE_OWNER:
					case TSS_DELEGATIONTYPE_KEY:
						result = obj_policy_set_delegation_type(hObject,
											ulAttrib);
						break;
					default:
						result = TSPERR(TSS_E_INVALID_ATTRIB_DATA);
					}
					break;
				case TSS_TSPATTRIB_POLDEL_INDEX:
					result = obj_policy_set_delegation_index(hObject, ulAttrib);
					break;
				case TSS_TSPATTRIB_POLDEL_PER1:
					result = obj_policy_set_delegation_per1(hObject, ulAttrib);
					break;
				case TSS_TSPATTRIB_POLDEL_PER2:
					result = obj_policy_set_delegation_per2(hObject, ulAttrib);
					break;
				default:
					result = TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
				}
				break;
#endif
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
				break;
		}
	} else if (obj_is_context(hObject)) {
		switch (attribFlag) {
			case TSS_TSPATTRIB_CONTEXT_SILENT_MODE:
				if (ulAttrib == TSS_TSPATTRIB_CONTEXT_NOT_SILENT)
					result = obj_context_set_mode(hObject, ulAttrib);
				else if (ulAttrib == TSS_TSPATTRIB_CONTEXT_SILENT) {
					if (obj_context_has_popups(hObject))
						return TSPERR(TSS_E_SILENT_CONTEXT);
					result = obj_context_set_mode(hObject, ulAttrib);
				} else
					return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
				break;
#ifdef TSS_BUILD_TRANSPORT
			case TSS_TSPATTRIB_CONTEXT_TRANSPORT:
				if (subFlag == TSS_TSPATTRIB_CONTEXTTRANS_CONTROL) {
					if (ulAttrib != TSS_TSPATTRIB_DISABLE_TRANSPORT &&
					    ulAttrib != TSS_TSPATTRIB_ENABLE_TRANSPORT)
						return TSPERR(TSS_E_INVALID_ATTRIB_DATA);

					result = obj_context_transport_set_control(hObject,
										   ulAttrib);
				} else if (subFlag == TSS_TSPATTRIB_CONTEXTTRANS_MODE) {
					switch (ulAttrib) {
						case TSS_TSPATTRIB_TRANSPORT_NO_DEFAULT_ENCRYPTION:
						case TSS_TSPATTRIB_TRANSPORT_DEFAULT_ENCRYPTION:
						case TSS_TSPATTRIB_TRANSPORT_AUTHENTIC_CHANNEL:
						case TSS_TSPATTRIB_TRANSPORT_EXCLUSIVE:
						case TSS_TSPATTRIB_TRANSPORT_STATIC_AUTH:
							break;
						default:
							return TSPERR(TSS_E_INVALID_ATTRIB_DATA);
					}

					result = obj_context_transport_set_mode(hObject, ulAttrib);
				} else
					return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);

				break;
#endif
			case TSS_TSPATTRIB_SECRET_HASH_MODE:
				result = obj_context_set_hash_mode(hObject, ulAttrib);
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
				break;
		}
	} else if (obj_is_tpm(hObject)) {
		switch (attribFlag) {
			case TSS_TSPATTRIB_TPM_CALLBACK_COLLATEIDENTITY:
			case TSS_TSPATTRIB_TPM_CALLBACK_ACTIVATEIDENTITY:
				if ((result = obj_tpm_set_cb11(hObject, attribFlag, subFlag,
							       ulAttrib)))
					return result;
				break;
#ifdef TSS_BUILD_AUDIT
			case TSS_TSPATTRIB_TPM_ORDINAL_AUDIT_STATUS:
				result = __tspi_audit_set_ordinal_audit_status(hObject, attribFlag,
									subFlag, ulAttrib);
				break;
#endif
			default:
				result = TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
				break;
		}
#ifdef TSS_BUILD_SEALX
	} else if (obj_is_encdata(hObject)) {
		if (attribFlag != TSS_TSPATTRIB_ENCDATA_SEAL)
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
		if (subFlag == TSS_TSPATTRIB_ENCDATASEAL_PROTECT_MODE) {
			if (ulAttrib != TSS_TSPATTRIB_ENCDATASEAL_NO_PROTECT &&
			    ulAttrib != TSS_TSPATTRIB_ENCDATASEAL_PROTECT)
				return TSPERR(TSS_E_INVALID_ATTRIB_DATA);

			result = obj_encdata_set_seal_protect_mode(hObject, ulAttrib);
		} else
			return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
#endif
#ifdef TSS_BUILD_DELEGATION
	} else if (obj_is_delfamily(hObject)) {
		switch (attribFlag) {
		case TSS_TSPATTRIB_DELFAMILY_STATE:
			switch (subFlag) {
			case TSS_TSPATTRIB_DELFAMILYSTATE_LOCKED:
				result = obj_delfamily_set_locked(hObject, (TSS_BOOL)ulAttrib, TRUE);
				break;
			case TSS_TSPATTRIB_DELFAMILYSTATE_ENABLED:
				result = obj_delfamily_set_enabled(hObject, (TSS_BOOL)ulAttrib, TRUE);
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
			break;
		default:
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
		}
#endif
	} else {
		if (obj_is_hash(hObject) || obj_is_pcrs(hObject))
			result = TSPERR(TSS_E_BAD_PARAMETER);
		else
			result = TSPERR(TSS_E_INVALID_HANDLE);
	}

	return result;
}

TSS_RESULT
Tspi_GetAttribUint32(TSS_HOBJECT hObject,	/* in */
		     TSS_FLAG attribFlag,	/* in */
		     TSS_FLAG subFlag,		/* in */
		     UINT32 * pulAttrib)	/* out */
{
	UINT32 attrib;
	TSS_RESULT result = TSS_SUCCESS;

	if (pulAttrib == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (obj_is_rsakey(hObject)) {
#ifdef TSS_BUILD_RSAKEY_LIST
		if (attribFlag == TSS_TSPATTRIB_KEY_REGISTER) {
			if (subFlag != 0)
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);

			if ((result = obj_rsakey_get_pstype(hObject, &attrib)))
				return result;

			if (attrib == TSS_PS_TYPE_USER)
				*pulAttrib = TSS_TSPATTRIB_KEYREGISTER_USER;
			else if (attrib == TSS_PS_TYPE_SYSTEM)
				*pulAttrib = TSS_TSPATTRIB_KEYREGISTER_SYSTEM;
			else
				*pulAttrib = TSS_TSPATTRIB_KEYREGISTER_NO;
		} else if (attribFlag == TSS_TSPATTRIB_KEY_INFO) {
			switch (subFlag) {
			case TSS_TSPATTRIB_KEYINFO_USAGE:
				if ((result = obj_rsakey_get_usage(hObject, pulAttrib)))
					return result;
				break;
			case TSS_TSPATTRIB_KEYINFO_MIGRATABLE:
				*pulAttrib = obj_rsakey_is_migratable(hObject);
				break;
			case TSS_TSPATTRIB_KEYINFO_REDIRECTED:
				*pulAttrib = obj_rsakey_is_redirected(hObject);
				break;
			case TSS_TSPATTRIB_KEYINFO_VOLATILE:
				*pulAttrib = obj_rsakey_is_volatile(hObject);
				break;
			case TSS_TSPATTRIB_KEYINFO_AUTHUSAGE:
				/* fall through */
			case TSS_TSPATTRIB_KEYINFO_AUTHDATAUSAGE:
				if ((result = obj_rsakey_get_authdata_usage(hObject, pulAttrib)))
					return result;
				break;
			case TSS_TSPATTRIB_KEYINFO_ALGORITHM:
				if ((result = obj_rsakey_get_alg(hObject, pulAttrib)))
					return result;
				break;
			case TSS_TSPATTRIB_KEYINFO_ENCSCHEME:
				if ((result = obj_rsakey_get_es(hObject, pulAttrib)))
					return result;
				break;
			case TSS_TSPATTRIB_KEYINFO_SIGSCHEME:
				if ((result = obj_rsakey_get_ss(hObject, pulAttrib)))
					return result;
				break;
			case TSS_TSPATTRIB_KEYINFO_KEYFLAGS:
				if ((result = obj_rsakey_get_flags(hObject, pulAttrib)))
					return result;
				break;
			case TSS_TSPATTRIB_KEYINFO_SIZE:
				if ((result = obj_rsakey_get_size(hObject, pulAttrib)))
					return result;
				break;
#ifdef TSS_BUILD_CMK
			case TSS_TSPATTRIB_KEYINFO_CMK:
				*pulAttrib = obj_rsakey_is_cmk(hObject);
				break;
#endif
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
		} else if (attribFlag == TSS_TSPATTRIB_RSAKEY_INFO) {
			if (subFlag == TSS_TSPATTRIB_KEYINFO_RSA_KEYSIZE) {
				if ((result = obj_rsakey_get_size(hObject, pulAttrib)))
					return result;
			} else if (subFlag == TSS_TSPATTRIB_KEYINFO_RSA_PRIMES) {
				if ((result = obj_rsakey_get_num_primes(hObject, pulAttrib)))
					return result;
			} else {
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
		} else if (attribFlag == TSS_TSPATTRIB_KEY_PCR_LONG) {
			if (subFlag == TSS_TSPATTRIB_KEYPCRLONG_LOCALITY_ATCREATION ||
			    subFlag == TSS_TSPATTRIB_KEYPCRLONG_LOCALITY_ATRELEASE) {
				result = obj_rsakey_get_pcr_locality(hObject, subFlag, pulAttrib);
			} else
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
		} else
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
#endif
#ifdef TSS_BUILD_NV
	} else if (obj_is_nvstore(hObject)) {
		switch (attribFlag) {
			case TSS_TSPATTRIB_NV_INDEX:
				if ((result = obj_nvstore_get_index(hObject, pulAttrib)))
					return result;
				break;
			case TSS_TSPATTRIB_NV_DATASIZE:
				if ((result = obj_nvstore_get_datasize(hObject, pulAttrib)))
					return result;
				break;
			case TSS_TSPATTRIB_NV_PERMISSIONS:
				if ((result = obj_nvstore_get_permission(hObject, pulAttrib)))
					return result;
				break;
			case TSS_TSPATTRIB_NV_STATE:
				switch (subFlag) {
					case TSS_TSPATTRIB_NVSTATE_READSTCLEAR:
						if ((result =
						     obj_nvstore_get_state_readstclear(hObject,
										       pulAttrib)))
							return result;
						break;
					case TSS_TSPATTRIB_NVSTATE_WRITEDEFINE:
						if ((result =
						     obj_nvstore_get_state_writedefine(hObject,
										       pulAttrib)))
							return result;
						break;
					case TSS_TSPATTRIB_NVSTATE_WRITESTCLEAR:
						if ((result =
						     obj_nvstore_get_state_writestclear(hObject,
											pulAttrib)))
							return result;
						break;
					default:
						return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
					}
				break;
			case TSS_TSPATTRIB_NV_PCR:
				switch (subFlag) {
					case TSS_TSPATTRIB_NVPCR_READLOCALITYATRELEASE:
						if ((result =
						     obj_nvstore_get_readlocalityatrelease(hObject,
										   pulAttrib)))
							return result;
						break;
					case TSS_TSPATTRIB_NVPCR_WRITELOCALITYATRELEASE:
						if ((result =
						     obj_nvstore_get_writelocalityatrelease(hObject,
										    pulAttrib)))
							return result;
						break;
					default:
						return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
					}
				break;
			case TSS_TSPATTRIB_KEYCONTROL_OWNEREVICT:
				if ((result = obj_rsakey_get_ownerevict(hObject, pulAttrib)))
					return result;
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
		}
#endif
	} else if (obj_is_policy(hObject)) {
		switch (attribFlag) {
			case TSS_TSPATTRIB_POLICY_CALLBACK_HMAC:
			case TSS_TSPATTRIB_POLICY_CALLBACK_XOR_ENC:
			case TSS_TSPATTRIB_POLICY_CALLBACK_TAKEOWNERSHIP:
			case TSS_TSPATTRIB_POLICY_CALLBACK_CHANGEAUTHASYM:
				if ((result = obj_policy_get_cb11(hObject, attribFlag, pulAttrib)))
					return result;
				break;
			case TSS_TSPATTRIB_POLICY_SECRET_LIFETIME:
				if ((result = obj_policy_get_lifetime(hObject, &attrib)))
					return result;

				if (subFlag == TSS_TSPATTRIB_POLICYSECRET_LIFETIME_ALWAYS) {
					if (attrib == TSS_TSPATTRIB_POLICYSECRET_LIFETIME_ALWAYS)
						*pulAttrib = TRUE;
					else
						*pulAttrib = FALSE;
				} else if (subFlag == TSS_TSPATTRIB_POLICYSECRET_LIFETIME_COUNTER) {
					if (attrib != TSS_TSPATTRIB_POLICYSECRET_LIFETIME_COUNTER)
						return TSPERR(TSS_E_BAD_PARAMETER);
					if ((result = obj_policy_get_counter(hObject, pulAttrib)))
						return result;
				} else if (subFlag == TSS_TSPATTRIB_POLICYSECRET_LIFETIME_TIMER) {
					if ((result =
					    obj_policy_get_secs_until_expired(hObject, pulAttrib)))
						return result;
				} else
					return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
				break;
			case TSS_TSPATTRIB_SECRET_HASH_MODE:
				if (subFlag == TSS_TSPATTRIB_SECRET_HASH_MODE_POPUP)
					result = obj_policy_get_hash_mode(hObject, pulAttrib);
				else
					return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
				break;
#ifdef TSS_BUILD_DELEGATION
			case TSS_TSPATTRIB_POLICY_DELEGATION_INFO:
				switch (subFlag) {
				case TSS_TSPATTRIB_POLDEL_TYPE:
					result = obj_policy_get_delegation_type(hObject,
							pulAttrib);
					break;
				case TSS_TSPATTRIB_POLDEL_INDEX:
					result = obj_policy_get_delegation_index(hObject,
							pulAttrib);
					break;
				case TSS_TSPATTRIB_POLDEL_PER1:
					result = obj_policy_get_delegation_per1(hObject,
							pulAttrib);
					break;
				case TSS_TSPATTRIB_POLDEL_PER2:
					result = obj_policy_get_delegation_per2(hObject,
							pulAttrib);
					break;
				case TSS_TSPATTRIB_POLDEL_LABEL:
					result = obj_policy_get_delegation_label(hObject,
							(BYTE *)pulAttrib);
					break;
				case TSS_TSPATTRIB_POLDEL_FAMILYID:
					result = obj_policy_get_delegation_familyid(hObject,
							pulAttrib);
					break;
				case TSS_TSPATTRIB_POLDEL_VERCOUNT:
					result = obj_policy_get_delegation_vercount(hObject,
							pulAttrib);
					break;
				default:
					result = TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
				}
				break;
			case TSS_TSPATTRIB_POLICY_DELEGATION_PCR:
				switch (subFlag) {
				case TSS_TSPATTRIB_POLDELPCR_LOCALITY:
					result = obj_policy_get_delegation_pcr_locality(hObject,
							pulAttrib);
					break;
				default:
					result = TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
				}
				break;
#endif
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
				break;
		}
	} else if (obj_is_context(hObject)) {
		switch (attribFlag) {
			case TSS_TSPATTRIB_CONTEXT_SILENT_MODE:
				if ((result = obj_context_get_mode(hObject, pulAttrib)))
					return result;
				break;
			case TSS_TSPATTRIB_SECRET_HASH_MODE:
				if (subFlag == TSS_TSPATTRIB_SECRET_HASH_MODE_POPUP)
					result = obj_context_get_hash_mode(hObject, pulAttrib);
				else
					return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
				break;
#ifdef TSS_BUILD_TRANSPORT
			case TSS_TSPATTRIB_CONTEXT_TRANSPORT:
				if (subFlag == TSS_TSPATTRIB_DISABLE_TRANSPORT ||
				    subFlag == TSS_TSPATTRIB_ENABLE_TRANSPORT) {
					result = obj_context_transport_get_control(hObject, subFlag,
										   pulAttrib);
				} else if (
					subFlag == TSS_TSPATTRIB_TRANSPORT_NO_DEFAULT_ENCRYPTION ||
					subFlag == TSS_TSPATTRIB_TRANSPORT_DEFAULT_ENCRYPTION ||
					subFlag == TSS_TSPATTRIB_TRANSPORT_AUTHENTIC_CHANNEL ||
					subFlag == TSS_TSPATTRIB_TRANSPORT_EXCLUSIVE ||
					subFlag == TSS_TSPATTRIB_TRANSPORT_STATIC_AUTH) {
					result = obj_context_transport_get_mode(hObject, subFlag,
										pulAttrib);
				} else
					return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
				break;
#endif
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
				break;
		}
	} else if (obj_is_tpm(hObject)) {
		switch (attribFlag) {
			case TSS_TSPATTRIB_TPM_CALLBACK_COLLATEIDENTITY:
			case TSS_TSPATTRIB_TPM_CALLBACK_ACTIVATEIDENTITY:
				if ((result = obj_tpm_get_cb11(hObject, attribFlag, pulAttrib)))
					return result;
				break;
			default:
				result = TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
				break;
		}
	} else if (obj_is_encdata(hObject)) {
#ifdef TSS_BUILD_SEALX
		if (attribFlag == TSS_TSPATTRIB_ENCDATA_SEAL) {
			if (subFlag == TSS_TSPATTRIB_ENCDATASEAL_PROTECT_MODE)
				result = obj_encdata_get_seal_protect_mode(hObject, pulAttrib);
			else
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
		} else if (attribFlag == TSS_TSPATTRIB_ENCDATA_PCR_LONG) {
			if (subFlag == TSS_TSPATTRIB_ENCDATAPCRLONG_LOCALITY_ATCREATION ||
			    subFlag == TSS_TSPATTRIB_ENCDATAPCRLONG_LOCALITY_ATRELEASE) {
				result = obj_encdata_get_pcr_locality(hObject, subFlag, pulAttrib);
			} else
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
		} else
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
#endif
#ifdef TSS_BUILD_DELEGATION
	} else if (obj_is_delfamily(hObject)) {
		switch (attribFlag) {
		case TSS_TSPATTRIB_DELFAMILY_STATE:
			switch (subFlag) {
			case TSS_TSPATTRIB_DELFAMILYSTATE_LOCKED:
				result = obj_delfamily_get_locked(hObject, (TSS_BOOL *)pulAttrib);
				break;
			case TSS_TSPATTRIB_DELFAMILYSTATE_ENABLED:
				result = obj_delfamily_get_enabled(hObject, (TSS_BOOL *)pulAttrib);
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
			break;
		case TSS_TSPATTRIB_DELFAMILY_INFO:
			switch (subFlag) {
			case TSS_TSPATTRIB_DELFAMILYINFO_LABEL:
				result = obj_delfamily_get_label(hObject, (BYTE *)pulAttrib);
				break;
			case TSS_TSPATTRIB_DELFAMILYINFO_VERCOUNT:
				result = obj_delfamily_get_vercount(hObject, pulAttrib);
				break;
			case TSS_TSPATTRIB_DELFAMILYINFO_FAMILYID:
				result = obj_delfamily_get_familyid(hObject, pulAttrib);
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
			break;
		default:
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
		}
#endif
	} else {
		if (obj_is_hash(hObject) || obj_is_pcrs(hObject))
			result = TSPERR(TSS_E_BAD_PARAMETER);
		else
			result = TSPERR(TSS_E_INVALID_HANDLE);
	}

	return result;
}

TSS_RESULT
Tspi_SetAttribData(TSS_HOBJECT hObject,		/* in */
		   TSS_FLAG attribFlag,		/* in */
		   TSS_FLAG subFlag,		/* in */
		   UINT32 ulAttribDataSize,	/* in */
		   BYTE * rgbAttribData)	/* in */
{
	TSS_RESULT result;
	BYTE *string = NULL;

	if (obj_is_rsakey(hObject)) {
#ifdef TSS_BUILD_RSAKEY_LIST
		if (attribFlag == TSS_TSPATTRIB_KEY_BLOB) {
			if (subFlag == TSS_TSPATTRIB_KEYBLOB_BLOB) {
				/* A TPM_KEY(12) structure, in blob form */
				result = obj_rsakey_set_tcpakey(hObject, ulAttribDataSize,
								rgbAttribData);
				if (result == TSS_SUCCESS)
					result = obj_rsakey_set_tcs_handle(hObject, 0);
			} else if (subFlag == TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY) {
				/* A TCPA_PUBKEY structure, in blob form */
				result = obj_rsakey_set_pubkey(hObject, FALSE, rgbAttribData);
			} else if (subFlag == TSS_TSPATTRIB_KEYBLOB_PRIVATE_KEY) {
				/* A blob, either encrypted or unencrypted */
				result = obj_rsakey_set_privkey(hObject, FALSE, ulAttribDataSize,
								rgbAttribData);
			} else {
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
		} else if (attribFlag == TSS_TSPATTRIB_RSAKEY_INFO) {
			if (subFlag == TSS_TSPATTRIB_KEYINFO_RSA_EXPONENT) {
				result = obj_rsakey_set_exponent(hObject, ulAttribDataSize,
								 rgbAttribData);
			} else if (subFlag == TSS_TSPATTRIB_KEYINFO_RSA_MODULUS) {
				result = obj_rsakey_set_modulus(hObject, ulAttribDataSize,
								rgbAttribData);
			} else {
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
#ifdef TSS_BUILD_CMK
		} else if (attribFlag == TSS_TSPATTRIB_KEY_CMKINFO) {
			if (subFlag == TSS_TSPATTRIB_KEYINFO_CMK_MA_APPROVAL) {
				result = obj_rsakey_set_msa_approval(hObject, ulAttribDataSize,
						rgbAttribData);
			} else if (subFlag == TSS_TSPATTRIB_KEYINFO_CMK_MA_DIGEST) {
				result = obj_rsakey_set_msa_digest(hObject, ulAttribDataSize,
						rgbAttribData);
			} else {
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
#endif
		} else {
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
		}
#endif
	} else if (obj_is_encdata(hObject)) {
#ifdef TSS_BUILD_ENCDATA_LIST
		if (attribFlag != TSS_TSPATTRIB_ENCDATA_BLOB)
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
		if (subFlag != TSS_TSPATTRIB_ENCDATABLOB_BLOB)
			return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);

		result = obj_encdata_set_data(hObject, ulAttribDataSize, rgbAttribData);
#endif
	} else if (obj_is_policy(hObject)) {
		switch (attribFlag) {
			case TSS_TSPATTRIB_POLICY_POPUPSTRING:
				if ((string = Trspi_UNICODE_To_Native(rgbAttribData,
								      NULL)) == NULL)
					return TSPERR(TSS_E_INTERNAL_ERROR);

				result = obj_policy_set_string(hObject,
							       ulAttribDataSize,
							       string);
				break;
			case TSS_TSPATTRIB_POLICY_CALLBACK_HMAC:
			case TSS_TSPATTRIB_POLICY_CALLBACK_XOR_ENC:
			case TSS_TSPATTRIB_POLICY_CALLBACK_TAKEOWNERSHIP:
			case TSS_TSPATTRIB_POLICY_CALLBACK_CHANGEAUTHASYM:
#ifdef TSS_BUILD_SEALX
			case TSS_TSPATTRIB_POLICY_CALLBACK_SEALX_MASK:
#endif
				result = obj_policy_set_cb12(hObject, attribFlag,
							     rgbAttribData);
				break;
#ifdef TSS_BUILD_DELEGATION
			case TSS_TSPATTRIB_POLICY_DELEGATION_INFO:
				switch (subFlag) {
				case TSS_TSPATTRIB_POLDEL_OWNERBLOB:
					result = obj_policy_set_delegation_blob(hObject,
							TSS_DELEGATIONTYPE_OWNER,
							ulAttribDataSize, rgbAttribData);
					break;
				case TSS_TSPATTRIB_POLDEL_KEYBLOB:
					result = obj_policy_set_delegation_blob(hObject,
							TSS_DELEGATIONTYPE_KEY,
							ulAttribDataSize, rgbAttribData);
					break;
				default:
					result = TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
				}
				break;
#endif
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
				break;
		}
	} else if (obj_is_hash(hObject)) {
#ifdef TSS_BUILD_HASH_LIST
		if (attribFlag != TSS_TSPATTRIB_HASH_IDENTIFIER)
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);

		if (subFlag != 0)
			return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);

		result = obj_hash_set_value(hObject, ulAttribDataSize, rgbAttribData);
#endif
	} else if (obj_is_tpm(hObject)) {
		switch (attribFlag) {
			case TSS_TSPATTRIB_TPM_CALLBACK_COLLATEIDENTITY:
			case TSS_TSPATTRIB_TPM_CALLBACK_ACTIVATEIDENTITY:
				result = obj_tpm_set_cb12(hObject, attribFlag,
							  rgbAttribData);
				break;
			case TSS_TSPATTRIB_TPM_CREDENTIAL:
				if (subFlag == TSS_TPMATTRIB_EKCERT ||
				    subFlag == TSS_TPMATTRIB_TPM_CC ||
				    subFlag == TSS_TPMATTRIB_PLATFORMCERT ||
				    subFlag == TSS_TPMATTRIB_PLATFORM_CC) {
					result = obj_tpm_set_cred(hObject, subFlag,
								  ulAttribDataSize, rgbAttribData);
				} else {
					return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
				}
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
				break;
		}
	} else if (obj_is_migdata(hObject)) {
#ifdef TSS_BUILD_CMK
		switch (attribFlag) {
		case TSS_MIGATTRIB_MIGRATIONBLOB:
			switch (subFlag) {
			case TSS_MIGATTRIB_MIG_MSALIST_PUBKEY_BLOB:
			case TSS_MIGATTRIB_MIG_AUTHORITY_PUBKEY_BLOB:
			case TSS_MIGATTRIB_MIG_DESTINATION_PUBKEY_BLOB:
			case TSS_MIGATTRIB_MIG_SOURCE_PUBKEY_BLOB:
				result = obj_migdata_set_migrationblob(hObject, subFlag,
						ulAttribDataSize, rgbAttribData);
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
			break;
		case TSS_MIGATTRIB_MIGRATIONTICKET:
			if (subFlag != 0)
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			result = obj_migdata_set_ticket_blob(hObject, ulAttribDataSize, rgbAttribData);
			break;
		case TSS_MIGATTRIB_AUTHORITY_DATA:
			switch (subFlag) {
			case TSS_MIGATTRIB_AUTHORITY_DIGEST:
			case TSS_MIGATTRIB_AUTHORITY_APPROVAL_HMAC:
			case TSS_MIGATTRIB_AUTHORITY_MSALIST:
				result = obj_migdata_set_authoritydata(hObject, subFlag,
						ulAttribDataSize, rgbAttribData);
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
			break;
		case TSS_MIGATTRIB_MIG_AUTH_DATA:
			switch (subFlag) {
			case TSS_MIGATTRIB_MIG_AUTH_AUTHORITY_DIGEST:
			case TSS_MIGATTRIB_MIG_AUTH_DESTINATION_DIGEST:
			case TSS_MIGATTRIB_MIG_AUTH_SOURCE_DIGEST:
				result = obj_migdata_set_migauthdata(hObject, subFlag,
						ulAttribDataSize, rgbAttribData);
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
			break;
		case TSS_MIGATTRIB_TICKET_DATA:
			switch (subFlag) {
			case TSS_MIGATTRIB_TICKET_SIG_DIGEST:
			case TSS_MIGATTRIB_TICKET_SIG_VALUE:
			case TSS_MIGATTRIB_TICKET_SIG_TICKET:
			case TSS_MIGATTRIB_TICKET_RESTRICT_TICKET:
				result = obj_migdata_set_ticketdata(hObject, subFlag,
						ulAttribDataSize, rgbAttribData);
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
			break;
		default:
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
			break;
		}
#endif
	} else {
		if (obj_is_pcrs(hObject) || obj_is_context(hObject))
			result = TSPERR(TSS_E_BAD_PARAMETER);
#ifdef TSS_BUILD_NV
		else if (obj_is_nvstore(hObject))
			result = TSPERR(TSS_E_BAD_PARAMETER);
#endif
		else
			result = TSPERR(TSS_E_INVALID_HANDLE);
	}

	return result;
}

TSS_RESULT
Tspi_GetAttribData(TSS_HOBJECT hObject,		/* in */
		   TSS_FLAG attribFlag,		/* in */
		   TSS_FLAG subFlag,		/* in */
		   UINT32 * pulAttribDataSize,	/* out */
		   BYTE ** prgbAttribData)	/* out */
{
	TSS_RESULT result;

	if (pulAttribDataSize == NULL || prgbAttribData == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (obj_is_rsakey(hObject)) {
#ifdef TSS_BUILD_RSAKEY_LIST
		if (attribFlag == TSS_TSPATTRIB_KEY_BLOB) {
			if (subFlag == TSS_TSPATTRIB_KEYBLOB_BLOB) {
				/* A TPM_KEY(12) structure, in blob form */
				result = obj_rsakey_get_blob(hObject, pulAttribDataSize,
							     prgbAttribData);
			} else if (subFlag == TSS_TSPATTRIB_KEYBLOB_PRIVATE_KEY) {
				/* A blob, either encrypted or unencrypted */
				result = obj_rsakey_get_priv_blob(hObject, pulAttribDataSize,
								  prgbAttribData);
			} else if (subFlag == TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY) {
				/* A TCPA_PUBKEY structure, in blob form */
				result = obj_rsakey_get_pub_blob(hObject, pulAttribDataSize,
								 prgbAttribData);
			} else {
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
		} else if (attribFlag == TSS_TSPATTRIB_KEY_INFO) {
			if (subFlag != TSS_TSPATTRIB_KEYINFO_VERSION)
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);

			result = obj_rsakey_get_version(hObject, pulAttribDataSize,
							prgbAttribData);
		} else if (attribFlag == TSS_TSPATTRIB_RSAKEY_INFO) {
			if (subFlag == TSS_TSPATTRIB_KEYINFO_RSA_EXPONENT) {
				result = obj_rsakey_get_exponent(hObject, pulAttribDataSize,
								 prgbAttribData);
			} else if (subFlag == TSS_TSPATTRIB_KEYINFO_RSA_MODULUS) {
				result = obj_rsakey_get_modulus(hObject, pulAttribDataSize,
								prgbAttribData);
			} else
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
		} else if (attribFlag == TSS_TSPATTRIB_KEY_UUID) {
			if (subFlag)
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);

			result = obj_rsakey_get_uuid(hObject, pulAttribDataSize, prgbAttribData);
		} else if (attribFlag == TSS_TSPATTRIB_KEY_PCR) {
			if (subFlag == TSS_TSPATTRIB_KEYPCR_DIGEST_ATCREATION ||
			    subFlag == TSS_TSPATTRIB_KEYPCR_DIGEST_ATRELEASE) {
				result = obj_rsakey_get_pcr_digest(hObject, TSS_PCRS_STRUCT_INFO,
								   subFlag, pulAttribDataSize,
								   prgbAttribData);
			} else if (subFlag == TSS_TSPATTRIB_KEYPCR_SELECTION) {
				result = obj_rsakey_get_pcr_selection(hObject, TSS_PCRS_STRUCT_INFO,
								      subFlag, pulAttribDataSize,
								      prgbAttribData);
			} else
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
		} else if (attribFlag == TSS_TSPATTRIB_KEY_PCR_LONG) {
			if (subFlag == TSS_TSPATTRIB_KEYPCRLONG_DIGEST_ATCREATION ||
			    subFlag == TSS_TSPATTRIB_KEYPCRLONG_DIGEST_ATRELEASE) {
				result = obj_rsakey_get_pcr_digest(hObject,
								   TSS_PCRS_STRUCT_INFO_LONG,
								   subFlag, pulAttribDataSize,
								   prgbAttribData);
			} else if (subFlag == TSS_TSPATTRIB_KEYPCRLONG_CREATION_SELECTION ||
			           subFlag == TSS_TSPATTRIB_KEYPCRLONG_RELEASE_SELECTION) {
				result = obj_rsakey_get_pcr_selection(hObject,
								      TSS_PCRS_STRUCT_INFO_LONG,
								      subFlag, pulAttribDataSize,
								      prgbAttribData);
			} else
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
#ifdef TSS_BUILD_CMK
		} else if (attribFlag == TSS_TSPATTRIB_KEY_CMKINFO) {
			if (subFlag == TSS_TSPATTRIB_KEYINFO_CMK_MA_APPROVAL) {
				result = obj_rsakey_get_msa_approval(hObject, pulAttribDataSize,
						prgbAttribData);
			} else if (subFlag == TSS_TSPATTRIB_KEYINFO_CMK_MA_DIGEST) {
				result = obj_rsakey_get_msa_digest(hObject, pulAttribDataSize,
						prgbAttribData);
			} else {
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
#endif
		} else
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
#endif
#ifdef TSS_BUILD_NV
	} else if (obj_is_nvstore(hObject)) {
		if (attribFlag == TSS_TSPATTRIB_NV_PCR) {
			switch (subFlag) {
				case TSS_TSPATTRIB_NVPCR_READDIGESTATRELEASE:
					if ((result = obj_nvstore_get_readdigestatrelease(hObject,
									pulAttribDataSize,
									prgbAttribData)))
							return result;
					break;
				case TSS_TSPATTRIB_NVPCR_READPCRSELECTION:
					if ((result = obj_nvstore_get_readpcrselection(
									hObject,
									pulAttribDataSize,
									prgbAttribData)))
						return result;
					break;
				case TSS_TSPATTRIB_NVPCR_WRITEDIGESTATRELEASE:
					if ((result = obj_nvstore_get_writedigestatrelease(hObject,
									pulAttribDataSize,
									prgbAttribData)))
						return result;
					break;
				case TSS_TSPATTRIB_NVPCR_WRITEPCRSELECTION:
					if ((result = obj_nvstore_get_writepcrselection(hObject,
									pulAttribDataSize,
									prgbAttribData)))
						return result;
					break;
				default:
					return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
		} else
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
#endif
	} else if (obj_is_encdata(hObject)) {
#ifdef TSS_BUILD_ENCDATA_LIST
		if (attribFlag == TSS_TSPATTRIB_ENCDATA_BLOB) {
			if (subFlag != TSS_TSPATTRIB_ENCDATABLOB_BLOB)
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);

			result = obj_encdata_get_data(hObject, pulAttribDataSize, prgbAttribData);
		} else if (attribFlag == TSS_TSPATTRIB_ENCDATA_PCR) {
			if (subFlag == TSS_TSPATTRIB_ENCDATAPCR_DIGEST_ATCREATION ||
			    subFlag == TSS_TSPATTRIB_ENCDATAPCR_DIGEST_RELEASE) {
				result = obj_encdata_get_pcr_digest(hObject, TSS_PCRS_STRUCT_INFO,
								    subFlag, pulAttribDataSize,
								    prgbAttribData);
			} else if (subFlag == TSS_TSPATTRIB_ENCDATAPCR_SELECTION) {
				result = obj_encdata_get_pcr_selection(hObject,
								       TSS_PCRS_STRUCT_INFO,
								       subFlag, pulAttribDataSize,
								       prgbAttribData);
			} else {
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
		} else if (attribFlag == TSS_TSPATTRIB_ENCDATA_PCR_LONG) {
			if (subFlag == TSS_TSPATTRIB_ENCDATAPCRLONG_CREATION_SELECTION ||
			    subFlag == TSS_TSPATTRIB_ENCDATAPCRLONG_RELEASE_SELECTION) {
				result = obj_encdata_get_pcr_selection(hObject,
								       TSS_PCRS_STRUCT_INFO_LONG,
								       subFlag, pulAttribDataSize,
								       prgbAttribData);
			} else if (subFlag == TSS_TSPATTRIB_ENCDATAPCRLONG_DIGEST_ATCREATION ||
				   subFlag == TSS_TSPATTRIB_ENCDATAPCRLONG_DIGEST_ATRELEASE) {
				result = obj_encdata_get_pcr_digest(hObject,
								    TSS_PCRS_STRUCT_INFO_LONG,
								    subFlag, pulAttribDataSize,
								    prgbAttribData);
			} else {
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
		} else {
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
		}
#endif
	} else if (obj_is_context(hObject)) {
		if (attribFlag != TSS_TSPATTRIB_CONTEXT_MACHINE_NAME)
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);

		if ((result = obj_context_get_machine_name_attrib(hObject,
								  pulAttribDataSize,
								  prgbAttribData)))
			return result;
	} else if (obj_is_policy(hObject)) {
		switch (attribFlag) {
			case TSS_TSPATTRIB_POLICY_CALLBACK_HMAC:
			case TSS_TSPATTRIB_POLICY_CALLBACK_XOR_ENC:
			case TSS_TSPATTRIB_POLICY_CALLBACK_TAKEOWNERSHIP:
			case TSS_TSPATTRIB_POLICY_CALLBACK_CHANGEAUTHASYM:
#ifdef TSS_BUILD_SEALX
			case TSS_TSPATTRIB_POLICY_CALLBACK_SEALX_MASK:
#endif
				result = obj_policy_get_cb12(hObject, attribFlag,
							     pulAttribDataSize, prgbAttribData);
				break;
			case TSS_TSPATTRIB_POLICY_POPUPSTRING:
				if ((result = obj_policy_get_string(hObject, pulAttribDataSize,
								    prgbAttribData)))
					return result;
				break;
#ifdef TSS_BUILD_DELEGATION
			case TSS_TSPATTRIB_POLICY_DELEGATION_INFO:
				switch (subFlag) {
				case TSS_TSPATTRIB_POLDEL_OWNERBLOB:
					result = obj_policy_get_delegation_blob(hObject,
							TSS_DELEGATIONTYPE_OWNER,
							pulAttribDataSize, prgbAttribData);
					break;
				case TSS_TSPATTRIB_POLDEL_KEYBLOB:
					result = obj_policy_get_delegation_blob(hObject,
							TSS_DELEGATIONTYPE_KEY,
							pulAttribDataSize, prgbAttribData);
					break;
				default:
					return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
				}
				break;
			case TSS_TSPATTRIB_POLICY_DELEGATION_PCR:
				switch (subFlag) {
				case TSS_TSPATTRIB_POLDELPCR_DIGESTATRELEASE:
					result = obj_policy_get_delegation_pcr_digest(hObject,
							pulAttribDataSize, prgbAttribData);
					break;
				case TSS_TSPATTRIB_POLDELPCR_SELECTION:
					result = obj_policy_get_delegation_pcr_selection(hObject,
							pulAttribDataSize, prgbAttribData);
					break;
				default:
					return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
				}
				break;
#endif
			default:
				result = TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
				break;
		}
	} else if (obj_is_tpm(hObject)) {
		switch (attribFlag) {
			case TSS_TSPATTRIB_TPM_CALLBACK_COLLATEIDENTITY:
			case TSS_TSPATTRIB_TPM_CALLBACK_ACTIVATEIDENTITY:
				result = obj_tpm_get_cb12(hObject, attribFlag,
							  pulAttribDataSize, prgbAttribData);
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
				break;
		}
	} else if (obj_is_migdata(hObject)) {
#ifdef TSS_BUILD_CMK
		switch (attribFlag) {
		case TSS_MIGATTRIB_MIGRATIONBLOB:
			switch (subFlag) {
			case TSS_MIGATTRIB_MIG_XOR_BLOB:
				result = obj_migdata_get_migrationblob(hObject, subFlag,
						pulAttribDataSize, prgbAttribData);
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
			break;
		case TSS_MIGATTRIB_AUTHORITY_DATA:
			switch (subFlag) {
			case TSS_MIGATTRIB_AUTHORITY_DIGEST:
			case TSS_MIGATTRIB_AUTHORITY_APPROVAL_HMAC:
			case TSS_MIGATTRIB_AUTHORITY_MSALIST:
				result = obj_migdata_get_authoritydata(hObject, subFlag,
						pulAttribDataSize, prgbAttribData);
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
			break;
		case TSS_MIGATTRIB_MIG_AUTH_DATA:
			switch (subFlag) {
			case TSS_MIGATTRIB_MIG_AUTH_AUTHORITY_DIGEST:
			case TSS_MIGATTRIB_MIG_AUTH_DESTINATION_DIGEST:
			case TSS_MIGATTRIB_MIG_AUTH_SOURCE_DIGEST:
				result = obj_migdata_get_migauthdata(hObject, subFlag,
						pulAttribDataSize, prgbAttribData);
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
			break;
		case TSS_MIGATTRIB_TICKET_DATA:
			switch (subFlag) {
			case TSS_MIGATTRIB_TICKET_SIG_TICKET:
				result = obj_migdata_get_ticketdata(hObject, subFlag,
						pulAttribDataSize, prgbAttribData);
				break;
			default:
				return TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			}
			break;
		default:
			return TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
			break;
		}
#endif
	} else {
		if (obj_is_hash(hObject) || obj_is_pcrs(hObject))
			result = TSPERR(TSS_E_BAD_PARAMETER);
		else
			result = TSPERR(TSS_E_INVALID_HANDLE);
	}

	return result;
}

