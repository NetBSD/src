
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2007
 *
 */

#ifndef _AUTHSESS_H_
#define _AUTHSESS_H_

struct authsess {
	TPM_AUTH *pAuth;
	TPM_AUTH auth;

	/* XOR masks created before each use of an OSAP session */
	TPM_ENCAUTH encAuthUse;
	TPM_ENCAUTH encAuthMig;

	TSS_HCONTEXT tspContext;
	TPM_COMMAND_CODE command;

	TSS_HOBJECT obj_parent;
	TSS_HPOLICY hUsageParent;
	UINT32 parentMode;
	TPM_SECRET parentSecret;
	TSS_CALLBACK cb_xor, cb_hmac, cb_sealx;

	TPM_ENTITY_TYPE entity_type;
	UINT32 entityValueSize;
	BYTE *entityValue;

	TSS_HOBJECT obj_child;
	TSS_HPOLICY hUsageChild, hMigChild;
	UINT32 uMode, mMode;

	/* Created during OSAP or DSAP protocol initiation */
	TPM_NONCE nonceOddxSAP;
	TPM_NONCE nonceEvenxSAP;
	TPM_HMAC sharedSecret;

	//MUTEX_DECLARE(lock);
	//struct authsess *next;
};

TSS_RESULT authsess_oiap_get(TSS_HOBJECT, TPM_COMMAND_CODE, TPM_DIGEST *, TPM_AUTH *);
TSS_RESULT authsess_oiap_put(TPM_AUTH *, TPM_DIGEST *);

TSS_RESULT authsess_xsap_init(TSS_HCONTEXT, TSS_HOBJECT, TSS_HOBJECT, TSS_BOOL, TPM_COMMAND_CODE, TPM_ENTITY_TYPE, struct authsess **);
TSS_RESULT authsess_xsap_hmac(struct authsess *, TPM_DIGEST *);
TSS_RESULT authsess_xsap_verify(struct authsess *, TPM_DIGEST *);
void       authsess_free(struct authsess *);

#define TSS_AUTH_POLICY_REQUIRED	TRUE
#define TSS_AUTH_POLICY_NOT_REQUIRED	FALSE

#endif
