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
 
#include <openpgpsdk/defs.h>
#include <openpgpsdk/types.h>
#include "openpgpsdk/keyring.h"
#include <openpgpsdk/armour.h>
#include "openpgpsdk/packet.h"
#include "openpgpsdk/packet-parse.h"
#include "openpgpsdk/util.h"
#include "openpgpsdk/std_print.h"
#include "openpgpsdk/readerwriter.h"
#include "../src/lib/parse_local.h"

#include "tests.h"

static char *filename_rsa_noarmour_nopassphrase_singlekey="enc_rsa_noarmour_np_singlekey.txt";
static char *filename_rsa_noarmour_passphrase_singlekey="enc_rsa_noarmour_pp_singlekey.txt";
static char *filename_rsa_armour_nopassphrase_singlekey="enc_rsa_armour_np_singlekey.txt";
static char *filename_rsa_armour_passphrase_singlekey="enc_rsa_armour_pp_singlekey.txt";
static char *filename_rsa_large_noarmour_nopassphrase="enc_rsa_large_noarmour_np.txt";
static char *filename_rsa_large_armour_nopassphrase="enc_rsa_large_armour_np.txt";

static ops_parse_cb_return_t
callback_ops_decrypt(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)
    {
    ops_parser_content_union_t* content=(ops_parser_content_union_t *)&content_->content;
    static ops_boolean_t skipping;

    OPS_USED(cbinfo);

//    ops_print_packet(content_);

    if(content_->tag != OPS_PTAG_CT_UNARMOURED_TEXT && skipping)
	{
	puts("...end of skip");
	skipping=ops_false;
	}

    switch(content_->tag)
	{
    case OPS_PTAG_CT_UNARMOURED_TEXT:
	printf("OPS_PTAG_CT_UNARMOURED_TEXT\n");
	if(!skipping)
	    {
	    puts("Skipping...");
	    skipping=ops_true;
	    }
	fwrite(content->unarmoured_text.data,1,
	       content->unarmoured_text.length,stdout);
	break;

    case OPS_PTAG_CT_PK_SESSION_KEY:
        return callback_pk_session_key(content_,cbinfo);

    case OPS_PARSER_CMD_GET_SECRET_KEY:
        return callback_cmd_get_secret_key(content_,cbinfo);

    case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
        return test_cb_get_passphrase(content_,cbinfo);

    case OPS_PTAG_CT_LITERAL_DATA_BODY:
        return callback_literal_data(content_,cbinfo);

    case OPS_PARSER_PTAG:
    case OPS_PTAG_CT_ARMOUR_HEADER:
    case OPS_PTAG_CT_ARMOUR_TRAILER:
    case OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY:
    case OPS_PTAG_CT_COMPRESSED:
    case OPS_PTAG_CT_LITERAL_DATA_HEADER:
    case OPS_PTAG_CT_SE_IP_DATA_BODY:
    case OPS_PTAG_CT_SE_IP_DATA_HEADER:
	// Ignore these packets 
	// They're handled in ops_parse_one_packet()
	// and nothing else needs to be done
	break;

    default:
        return callback_general(content_,cbinfo);
	}

    return OPS_RELEASE_MEMORY;
    }

/* Decryption suite initialization.
 * Create temporary directory.
 * Create temporary test files.
 */

int init_suite_rsa_encrypt(void)
    {
    // Create RSA test files

    create_small_testfile(filename_rsa_noarmour_nopassphrase_singlekey);
    create_small_testfile(filename_rsa_noarmour_passphrase_singlekey);
    create_small_testfile(filename_rsa_armour_nopassphrase_singlekey);
    create_small_testfile(filename_rsa_armour_passphrase_singlekey);
    create_large_testfile(filename_rsa_large_noarmour_nopassphrase);
    create_large_testfile(filename_rsa_large_armour_nopassphrase);

    // Return success
    return 0;
    }

int clean_suite_rsa_encrypt(void)
    {
	
    ops_finish();

    reset_vars();

    return 0;
    }

static int test_rsa_decrypt(const char *encfile, const char*testtext, const int use_armour)
    {
    int fd=0;
    ops_parse_info_t *pinfo=NULL;
    ops_memory_t* mem_out=NULL;
    int rtn=0;

    ops_setup_file_read(&pinfo, encfile,
                        NULL, /* arg */
                        callback_ops_decrypt,
                        ops_false /* accumulate */
                        );

    // setup for writing parsed data to mem_out */
    ops_setup_memory_write(&pinfo->cbinfo.cinfo, &mem_out, 128);

    // other setup
    if (use_armour)
        ops_reader_push_dearmour(pinfo);

    // do it
    rtn=ops_parse_and_print_errors(pinfo);
    CU_ASSERT(rtn==1);

    // Tidy up

    close(fd);
    
    // File contents should match

    CU_ASSERT(memcmp(ops_memory_get_data(mem_out),testtext,ops_memory_get_length(mem_out))==0);

    return rtn;
    }

static void test_rsa_encrypt(const int use_armour, const char* filename, const ops_keydata_t *pub_key)
    {
    char cmd[MAXBUF+1];
    char myfile[MAXBUF+1];
    char encrypted_file[MAXBUF+1];
    char decrypted_file[MAXBUF+1];
    char* testtext;
    char *suffix= use_armour ? "asc" : "ops";
    int rtn=0;
    char pp[MAXBUF];
    ops_boolean_t allow_overwrite=ops_true;
    int repeats=10;

    // filenames
    snprintf(myfile,sizeof myfile,"%s/%s",dir,filename);
    snprintf(encrypted_file,sizeof encrypted_file,"%s/%s.%s",dir,filename,suffix);

    ops_encrypt_file(myfile,encrypted_file, pub_key, use_armour, allow_overwrite);

    /*
     * Test results
     */

    // File contents should match - check with GPG
        
    if (pub_key==alpha_pub_keydata)
        pp[0]='\0';
    else if (pub_key==bravo_pub_keydata)
        snprintf(pp,sizeof pp," --passphrase %s ", bravo_passphrase);
    snprintf(decrypted_file,sizeof decrypted_file,"%s/decrypted_%s",dir,filename);
    snprintf(cmd,sizeof cmd,"cat %s | %s --decrypt --output=%s %s",encrypted_file, gpgcmd, decrypted_file, pp);
    //printf("cmd: %s\n", cmd);
    rtn=run(cmd);
    CU_ASSERT(rtn==0);
    CU_ASSERT(file_compare(myfile,decrypted_file)==0);

    // File contents should match - checking with OPS
        
    testtext=create_testtext(filename,repeats);
    test_rsa_decrypt(encrypted_file,testtext, use_armour);

    // tidy up
    free(testtext);
    }

static void test_rsa_encrypt_noarmour_nopassphrase_singlekey(void)
    {
    test_rsa_encrypt(OPS_UNARMOURED,filename_rsa_noarmour_nopassphrase_singlekey, alpha_pub_keydata);
    }

static void test_rsa_encrypt_noarmour_passphrase_singlekey(void)
    {
    test_rsa_encrypt(OPS_UNARMOURED,filename_rsa_noarmour_passphrase_singlekey,bravo_pub_keydata);
    }

static void test_rsa_encrypt_armour_nopassphrase_singlekey(void)
    {
    test_rsa_encrypt(OPS_ARMOURED,filename_rsa_armour_nopassphrase_singlekey,alpha_pub_keydata);
    }

static void test_rsa_encrypt_armour_passphrase_singlekey(void)
    {
    test_rsa_encrypt(OPS_ARMOURED,filename_rsa_armour_passphrase_singlekey,bravo_pub_keydata);
    }

static void test_rsa_encrypt_large_noarmour_nopassphrase(void)
    {
    test_rsa_encrypt(OPS_UNARMOURED,filename_rsa_large_noarmour_nopassphrase, alpha_pub_keydata);
    }

static void test_rsa_encrypt_large_armour_nopassphrase(void)
    {
    test_rsa_encrypt(OPS_ARMOURED,filename_rsa_large_armour_nopassphrase, alpha_pub_keydata);
    }

/*
  FUTURE:
static void test_todo(void)
    {
    CU_FAIL("Test TODO: Use AES128");
    CU_FAIL("Test TODO: Use AES256");
    CU_FAIL("Test TODO: Encrypt to multiple keys in same keyring");
    CU_FAIL("Test TODO: Encrypt to multiple keys where my keys is (a) first key in list; (b) last key in list; (c) in the middle of the list");
    CU_FAIL("Test TODO: Encrypt to users with different preferences");
    }
*/

static int add_tests(CU_pSuite suite)
    {
    // add tests to suite
    
    if (NULL == CU_add_test(suite, "Unarmoured, single key, no passphrase", test_rsa_encrypt_noarmour_nopassphrase_singlekey))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Unarmoured, single key, passphrase", test_rsa_encrypt_noarmour_passphrase_singlekey))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Armoured, single key, no passphrase", test_rsa_encrypt_armour_nopassphrase_singlekey))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Armoured, single key, passphrase", test_rsa_encrypt_armour_passphrase_singlekey))
	    return 0;

    if (NULL == CU_add_test(suite, "Large, no armour, no passphrase", test_rsa_encrypt_large_noarmour_nopassphrase))
	    return 0;

    if (NULL == CU_add_test(suite, "Large, armour, no passphrase", test_rsa_encrypt_large_armour_nopassphrase))
	    return 0;

    /*
    if (NULL == CU_add_test(suite, "Tests to be implemented", test_todo))
	    return 0;
    */

    return 1;
    }
    
CU_pSuite suite_rsa_encrypt()
    {
    CU_pSuite suite = NULL;

    suite = CU_add_suite("RSA Encryption Suite", init_suite_rsa_encrypt, clean_suite_rsa_encrypt);
    if (!suite)
	    return NULL;

    if (!add_tests(suite))
        return NULL;

    return suite;
    }

// EOF
