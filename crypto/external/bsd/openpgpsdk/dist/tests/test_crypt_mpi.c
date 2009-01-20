/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. 
 * 
 * You may obtain a copy of the License at 
 *     http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * 
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CUnit/Basic.h"

#include <openssl/cast.h>

#include "tests.h"
#include "openpgpsdk/types.h"
#include "openpgpsdk/keyring.h"
#include "../src/lib/keyring_local.h"
#include "openpgpsdk/packet.h"
#include "openpgpsdk/create.h"

static const ops_keydata_t *pubkey;
static const ops_keydata_t *seckey;

int init_suite_crypt_mpi(void)
    {
    pubkey=ops_keyring_find_key_by_userid(&pub_keyring,alpha_user_id);
    seckey=ops_keyring_find_key_by_userid(&sec_keyring,alpha_user_id);

    // Return success
    return 0;
    }

int clean_suite_crypt_mpi(void)
    {
	
    ops_finish();

    reset_vars();

    return 0;
    }

void test_crypt_mpi(void)
    {
    // hardcoded using CAST
#define BSZ (CAST_KEY_LENGTH+1+2)

    unsigned char in[BSZ];
    unsigned char out[BSZ];

    ops_boolean_t rtn;
    
    ops_pk_session_key_t *encrypted_pk_session_key=NULL;

    encrypted_pk_session_key=ops_create_pk_session_key(pubkey);

    // the encrypted_mpi is now in session_key->parameters.rsa.encrypted_m

    // decrypt it
    rtn=ops_decrypt_and_unencode_mpi(out,BSZ, encrypted_pk_session_key->parameters.rsa.encrypted_m, &seckey->key.skey);

    // is it the same?
    CU_ASSERT(memcmp((char *)in,(char *)out,sizeof(in))==0);
    }

CU_pSuite suite_crypt_mpi()
    {
    CU_pSuite suite = NULL;

    suite = CU_add_suite("Crypt MPI Suite", init_suite_crypt_mpi, clean_suite_crypt_mpi);
    if (!suite)
	    return NULL;

    // add tests to suite
    
    if (NULL == CU_add_test(suite, "encrypt_mpi, then decrypt_mpi", test_crypt_mpi))
	    return NULL;
    
    return suite;
    }
