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

#include "openpgpsdk/defs.h"
#include "openpgpsdk/keyring.h"
#include "openpgpsdk/crypto.h"
#include "openpgpsdk/packet.h"
#include "openpgpsdk/validate.h"
#include "openpgpsdk/readerwriter.h"
#include "../src/lib/keyring_local.h"

#include "tests.h"

//static int debug=0;

int init_suite_rsa_keys(void)
    {
    return 0;
    }

int clean_suite_rsa_keys(void)
    {
    return 0;
    }

static void test_rsa_keys_generate_keypair(void)
    {
    ops_keydata_t* keydata=ops_keydata_new();
    CU_ASSERT(ops_rsa_generate_keypair(1024,65537,keydata)==ops_true);
    ops_keydata_free(keydata);
    }

static void test_rsa_keys_selfsign_keypair(void)
    {
    ops_user_id_t uid;
    uid.user_id=(unsigned char *)"Test User <test@nowhere.com>";

    ops_keydata_t* keydata=NULL;

    keydata=ops_rsa_create_selfsigned_keypair(1024,17,&uid);

    CU_ASSERT(keydata != NULL);

    ops_keydata_free(keydata);
    }

static ops_parse_cb_return_t
cb_get_passphrase(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo __attribute__((unused)))
    {
    const ops_parser_content_union_t *content=&content_->content;
    //    validate_key_cb_arg_t *arg=ops_parse_cb_get_arg(cbinfo);
    //    ops_error_t **errors=ops_parse_cb_get_errors(cbinfo);

    switch(content_->tag)
        {
    case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
        /*
          Doing this so the test can be automated.
        */
        *(content->secret_key_passphrase.passphrase)=ops_malloc_passphrase("hello");
        return OPS_KEEP_MEMORY;
        break;

    default:
        break;
        }

    return OPS_RELEASE_MEMORY;
    }

static void verify_keypair(ops_boolean_t armoured)
    {
    ops_user_id_t uid;
    ops_keydata_t* keydata=NULL;
    ops_create_info_t* cinfo=NULL;
    ops_validate_result_t* result=NULL;
    ops_keyring_t pub_keyring;
    ops_keyring_t sec_keyring;
    const ops_public_key_t* pub_key=NULL;
    const ops_secret_key_t* sec_key=NULL;
    const char* pp="hello";
    const unsigned char *passphrase=(unsigned char *)pp;
    const size_t pplen=strlen(pp);
    char filename[MAXBUF+1];
    int fd=0;
    char *suffix = armoured ? "asc" : "ops";
    char cmd[MAXBUF+1];
    int rtn=0;
    ops_boolean_t overwrite=ops_true;
    char* userid="Test User 2<test2@nowhere.com>";
    const unsigned char* keyid;

    memset(&pub_keyring, '\0', sizeof pub_keyring);
    memset(&sec_keyring, '\0', sizeof sec_keyring);


    uid.user_id=(unsigned char *) userid;

    keydata=ops_rsa_create_selfsigned_keypair(1024,65537,&uid);
    CU_ASSERT(keydata != NULL);
    pub_key=ops_get_public_key_from_data(keydata);
    sec_key=ops_get_secret_key_from_data(keydata);
    keyid=ops_get_key_id(keydata);

    /*
     * pub key
     */

    snprintf(filename,MAXBUF,"%s/%s.%s",dir,"ops_transferable_public_key",suffix);
    fd=ops_setup_file_write(&cinfo, filename,overwrite);
    ops_write_transferable_public_key(keydata,armoured,cinfo);
    ops_teardown_file_write(cinfo,fd);

    /*
     * Validate public key with OPS
     */

    // generate keyring from this file
    ops_keyring_read_from_file(&pub_keyring, armoured, filename);

    // validate keyring

    result=ops_mallocz(sizeof(*result));

    CU_ASSERT(ops_validate_all_signatures(result, &pub_keyring, NULL)==ops_true);
    CU_ASSERT(result->valid_count==1);

    CU_ASSERT(memcmp(result->valid_sigs[0].signer_id,keyid,OPS_KEY_ID_SIZE)==0);
    CU_ASSERT(result->invalid_count==0);
    CU_ASSERT(result->unknown_signer_count==0);

    ops_validate_result_free(result);

    // Validate public key with GPG

    snprintf(cmd, sizeof cmd, "cat %s | %s --import --no-allow-non-selfsigned-uid", filename, gpgcmd);
    rtn=run(cmd);
    CU_ASSERT(rtn==0); 

    /*
     * sec key
     */
    
    result=ops_mallocz(sizeof(*result));

    snprintf(filename,MAXBUF,"%s/%s.%s",dir,"ops_transferable_secret_key",suffix);
    fd=ops_setup_file_write(&cinfo, filename,overwrite);
    ops_write_transferable_secret_key(keydata,passphrase,pplen,armoured,cinfo);
    ops_teardown_file_write(cinfo,fd);

    // generate keyring from this file
    ops_keyring_read_from_file(&sec_keyring, armoured, filename);

    // validate keyring

    result=ops_mallocz(sizeof(*result));

    CU_ASSERT(ops_validate_all_signatures(result, &sec_keyring, cb_get_passphrase)==ops_true);
    CU_ASSERT(result->valid_count==1);
    CU_ASSERT(result->invalid_count==0);
    CU_ASSERT(result->unknown_signer_count==0);

    ops_validate_result_free(result);

    // validate with GPG
    snprintf(cmd, sizeof cmd, "cat %s | %s --import --no-allow-non-selfsigned-uid", filename, gpgcmd);
    rtn=run(cmd);
    CU_ASSERT(rtn==0); 

    // cleanup
    ops_keydata_free(keydata);
    }

static void test_rsa_keys_verify_keypair(void)
    {
    verify_keypair(OPS_UNARMOURED);
    }

static void test_rsa_keys_read_from_file(void)
    {
    ops_keyring_t keyring;
    char filename[MAXBUF+1];
    snprintf(filename,MAXBUF,"%s/%s", dir, "pubring.gpg");

    memset(&keyring, '\0', sizeof keyring);

    ops_keyring_read_from_file(&keyring, OPS_UNARMOURED, filename);
    ops_keyring_free(&keyring);
    }

static void test_rsa_keys_verify_armoured_keypair(void)
    {
    verify_keypair(OPS_ARMOURED);
    }

static void test_rsa_keys_verify_keypair_fail(void)
    {
    ops_user_id_t uid1, uid2;
    ops_keydata_t* keydata=NULL;
    const ops_keydata_t* keydata1;
    const ops_keydata_t* keydata2;
    const ops_keydata_t* keydata3;
    ops_create_info_t* cinfo=NULL;
    ops_validate_result_t* result=NULL;
    ops_keyring_t keyring1;
    ops_keyring_t keyring2;
    ops_keyring_t keyring3;
    char filename[MAXBUF+1];
    int fd=0;
    char* name1="First User<user1@nowhere.com>";
    uid1.user_id=(unsigned char *)name1;
    char* name2="Second<user2@nowhere.com>";
    uid2.user_id=(unsigned char *)name2;
    char cmd[MAXBUF+1];
    int rtn=0;
    ops_boolean_t overwrite=ops_true;

    memset(&keyring1, '\0', sizeof keyring1);
    memset(&keyring2, '\0', sizeof keyring2);
    memset(&keyring3, '\0', sizeof keyring3);

    /*
     * Create keys and keyrings
     * Keyring 1: contains good self-signed key 1
     * Keyring 2: contains good self-signed key 2
     * Keyring 3: contains self-signed key 3 with bad sig
     *
     * Use armoured keys
     */

    // Keyring 1
    keydata=ops_rsa_create_selfsigned_keypair(1024,65537,&uid1);
    CU_ASSERT(keydata != NULL);
    snprintf(filename,MAXBUF,"%s/%s",dir,"transferable_public_key_1");
    fd=ops_setup_file_write(&cinfo, filename,overwrite);
    ops_write_transferable_public_key(keydata,OPS_ARMOURED,cinfo);
    ops_teardown_file_write(cinfo,fd);
    ops_keyring_read_from_file(&keyring1, OPS_ARMOURED, filename);

    // Keyring 2
    keydata=ops_rsa_create_selfsigned_keypair(1024,65537,&uid2);
    CU_ASSERT(keydata2 != NULL);
    snprintf(filename,MAXBUF,"%s/%s",dir,"transferable_public_key_2");
    fd=ops_setup_file_write(&cinfo, filename,overwrite);
    ops_write_transferable_public_key(keydata,OPS_ARMOURED,cinfo);
    ops_teardown_file_write(cinfo,fd);
    ops_keyring_read_from_file(&keyring2, OPS_ARMOURED, filename);

    // Keyring 3
    assert(keydata->nsigs==1); // keydata is the key used in keyring2
    sigpacket_t *sig=&keydata->sigs[0];
    int target=sig->packet->length/2;
    unsigned orig=sig->packet->raw[target];
    // change a byte somewhere near the middle
    sig->packet->raw[target] = ~orig;
    assert(orig != sig->packet->raw[target]);

    snprintf(filename,MAXBUF,"%s/%s",dir,"transferable_public_key_3_bad");
    fd=ops_setup_file_write(&cinfo, filename, overwrite);
    ops_write_transferable_public_key(keydata,OPS_ARMOURED,cinfo);
    ops_teardown_file_write(cinfo,fd);
    ops_keyring_read_from_file(&keyring3, OPS_ARMOURED, filename);

    /*
     * Test: Validate key against keyring without this key in it. 
     * should fail as unknown
     */
    result=ops_mallocz(sizeof(*result));
    keydata1=ops_keyring_find_key_by_userid(&keyring1, name1);
    assert(keydata1);

    CU_ASSERT(ops_validate_key_signatures(result, keydata1, &keyring2, NULL)==ops_false);

    CU_ASSERT(result->valid_count==0);
    CU_ASSERT(result->invalid_count==0);
    CU_ASSERT(result->unknown_signer_count==1);
    ops_validate_result_free(result);

    /*
     * Test: Check correct behaviour if signature is bad
     * Change signature on key to be incorrect.
     * Then validate key against keyring with this key on it.
     * Should fail as invalid.
     */

    result=ops_mallocz(sizeof(ops_validate_result_t));
    keydata3=ops_keyring_find_key_by_userid(&keyring3, name2);
    assert(keydata3);

    CU_ASSERT(ops_validate_key_signatures(result, keydata3, &keyring3, NULL)==ops_false);

    CU_ASSERT(result->valid_count==0);
    CU_ASSERT(result->invalid_count==1);
    CU_ASSERT(result->unknown_signer_count==0);
    ops_validate_result_free(result);

    // validate with GPG - should fail
    snprintf(cmd, sizeof cmd, "cat %s | %s --import --no-allow-non-selfsigned-uid", filename, gpgcmd);
    rtn=run(cmd);
    CU_ASSERT(rtn!=0); 

    /*
     * cleanup
     */
    ops_keydata_free(keydata);
    /*
      ops_keydata_free(keydata1);
    ops_keydata_free(keydata2);
    ops_keydata_free(keydata3);
    */
    }

#ifdef XXX
static void test_rsa_keys_todo(void)
    {
    /*
     * Key expiry and revocation checking can be done at the 
     * application level by checking the set of valid keys returned.
     */

    // expired
    //CU_FAIL("Test TODO: Check for key expiry");

    // revoked
    //CU_FAIL("Test TODO: Check for key revocation");
    }
#endif

CU_pSuite suite_rsa_keys()
    {
    CU_pSuite suite=NULL;

    // add suite
    suite=CU_add_suite("RSA Keys Suite", init_suite_rsa_keys, clean_suite_rsa_keys);
    if (!suite)
        return NULL;

    // add tests to suite

    if (NULL == CU_add_test(suite, "Generate key pair", test_rsa_keys_generate_keypair))
        return NULL;

    if (NULL == CU_add_test(suite, "Self-sign key pair", test_rsa_keys_selfsign_keypair))
        return NULL;

    if (NULL == CU_add_test(suite, "Verify self-signed key pair", test_rsa_keys_verify_keypair))
        return NULL;

    if (NULL == CU_add_test(suite, "Verify self-signed key pair (armoured)", test_rsa_keys_verify_armoured_keypair))
        return NULL;

    if (NULL == CU_add_test(suite, "Verify self-signed key pair fails", test_rsa_keys_verify_keypair_fail))
        return NULL;

    if (NULL == CU_add_test(suite, "Read keyring from file", test_rsa_keys_read_from_file))
        return NULL;

    /*
    if (NULL == CU_add_test(suite, "TODO", test_rsa_keys_todo))
        return NULL;
    */

    // return
    return suite;
    }

// EOF
