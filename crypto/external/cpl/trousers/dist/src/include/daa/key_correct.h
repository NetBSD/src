
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifndef KEY_CORRECT_H_
#define KEY_CORRECT_H_

#include "daa/daa_structs.h"
#include "daa/daa_parameter.h"
#include "tsplog.h"

   /**
   * Verifies if the parameters Z,R0,R1,RReceiver and RIssuer of the public key
   * were correctly computed.
   *
   * @param pk
   *          the public key, which one wants to verfy.
   */
TSS_RESULT
is_pk_correct( TSS_DAA_PK_internal *public_key,
		TSS_DAA_PK_PROOF_internal *proof,
		int *isCorrect
);

#endif /*KEY_CORRECTNESS_PROOF_H_*/
