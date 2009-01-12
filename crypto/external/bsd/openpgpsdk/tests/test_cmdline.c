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

#include "tests.h"

#include "CUnit/Basic.h"

char openpgp[MAXBUF+1];
char cmd[MAXBUF+1];

char *testfile_encrypt="cmdline_encrypt.txt";
char *testfile_sign="cmdline_sign.txt";
char *testfile_clearsign="cmdline_clearsign.txt";

const char* bad_user_id="no one by this name <noone@nowwhere.com>";

int init_suite_cmdline(void)
    {
    snprintf(openpgp,MAXBUF,"%s/../../src/app/openpgp", dir);

    create_small_testfile(testfile_encrypt);
    create_small_testfile(testfile_sign);
    create_small_testfile(testfile_clearsign);

    // Return success
    return 0;
    }

int clean_suite_cmdline(void)
    {
    return 0;
    }

static void test_list_keys(void)
    {
    snprintf(cmd, MAXBUF, "%s/../../src/app/openpgp --list-keys --homedir=%s --keyring='pubring.gpg'", dir, dir);
    CU_ASSERT(run(cmd)==0);

    snprintf(cmd, MAXBUF, "%s/../../src/app/openpgp --list-keys --homedir=%s --keyring='secring.gpg'", dir, dir);
    CU_ASSERT(run(cmd)==0);

    // This one should fail
    snprintf(cmd, MAXBUF, "%s/../../src/app/openpgp --list-keys --homedir=%s --keyring='badring.gpg'", dir, dir);
    CU_ASSERT(run(cmd)!=0);
    }

static void test_find_key(void)
    {
    int rtn=0;

    snprintf(cmd, MAXBUF, "%s --find-key --homedir=%s --keyring='pubring.gpg' --userid='%s'", openpgp, dir, alpha_user_id);
    rtn=run(cmd);
    CU_ASSERT(rtn>0);

    // should be found in secring
    snprintf(cmd, MAXBUF, "%s --find-key --homedir=%s --keyring='secring.gpg' --userid='%s'", openpgp, dir, alpha_user_id);
    rtn=run(cmd);
    CU_ASSERT(rtn>1);

    // this one should not succeed
    snprintf(cmd, MAXBUF, "%s --find-key --homedir=%s --keyring='pubring.gpg' --userid='%s'", openpgp, dir, bad_user_id);
    rtn=run(cmd);
    CU_ASSERT(rtn==0);
    }

static void test_exportkey(void)
    {
    int rtn=0;
    snprintf(cmd, MAXBUF, "%s --export-key --homedir=%s --keyring='pubring.gpg' --userid='%s'", openpgp,dir, alpha_user_id);
    rtn=run(cmd);
    CU_ASSERT(rtn==0);

    snprintf(cmd, MAXBUF, "%s --export-key --homedir=%s --keyring='pubring.gpg' --userid='%s'", openpgp,dir, bravo_user_id);
    rtn=run(cmd);
    CU_ASSERT(rtn==0);

    snprintf(cmd, MAXBUF, "%s --export-key --homedir=%s --keyring='pubring.gpg' --userid='%s'", openpgp,dir, bad_user_id);
    rtn=run(cmd);
    CU_ASSERT(rtn!=0);
    }

static void test_generate_key(void)
    {
    int rtn=0;

    snprintf(cmd, MAXBUF, "%s --generate-key --homedir=%s --userid='%s'", openpgp, dir, charlie_user_id);
    rtn=run(cmd);
    CU_ASSERT(rtn==0);
    }

static void test_encrypt(void)
    {
    int rtn=0;

    snprintf(cmd, MAXBUF, "%s --encrypt --homedir=%s --userid='%s' --file=%s/%s", openpgp, dir, alpha_user_id, dir, testfile_encrypt);
    rtn=run(cmd);
    CU_ASSERT(rtn==0);
    }

static void test_decrypt(void)
    {
    int rtn=0;

    // move original file
    snprintf(cmd, MAXBUF, "mv %s/%s %s/copy.%s", dir, testfile_encrypt, dir, testfile_encrypt);
    CU_ASSERT(run(cmd)==0);

    // now decrypt the encrypted file to recreate it
    snprintf(cmd, MAXBUF, "echo | %s --decrypt --homedir=%s --userid='%s' --file=%s/%s.gpg", openpgp, dir, alpha_user_id, dir, testfile_encrypt);
    rtn=run(cmd);
    CU_ASSERT(rtn==0);

    // now compare the original and the decrypted file
    snprintf(cmd, MAXBUF, "diff %s/%s %s/copy.%s", dir, testfile_encrypt, dir, testfile_encrypt);
    rtn=run(cmd);
    CU_ASSERT(rtn==0);
    }

static void test_sign(void)
    {
    snprintf(cmd, MAXBUF, "%s --sign --homedir=%s --userid='%s' --file=%s/%s --armour", openpgp, dir, alpha_user_id, dir, testfile_sign);
    CU_ASSERT(run(cmd)==0);
    }

static void test_clearsign(void)
    {
    snprintf(cmd, MAXBUF, "%s --clearsign --homedir=%s --userid='%s' --file=%s/%s ", openpgp, dir, alpha_user_id, dir, testfile_clearsign);
    CU_ASSERT(run(cmd)==0);
    }

static void test_verify(void)
    {
    snprintf(cmd, MAXBUF, "%s --verify --homedir=%s --file=%s/%s.asc --armour", openpgp, dir, dir, testfile_sign);
    CU_ASSERT(run(cmd)==0);

    snprintf(cmd, MAXBUF, "%s --verify --homedir=%s --file=%s/%s.asc --armour", openpgp, dir, dir, testfile_clearsign);
    CU_ASSERT(run(cmd)==0);
    }

static int add_tests(CU_pSuite suite)
    {
    // add tests to suite
    
    if (NULL == CU_add_test(suite, "List Keys", test_list_keys))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Find Key", test_find_key))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Export Key", test_exportkey))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Generate Key", test_generate_key))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Encrypt", test_encrypt))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Decrypt", test_decrypt))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Sign", test_sign))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Clearsign", test_clearsign))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Verify", test_verify))
	    return 0;
    
    return 1;
    }

CU_pSuite suite_cmdline()
{
    CU_pSuite suite = NULL;

    suite = CU_add_suite("Command Line Suite", init_suite_cmdline, clean_suite_cmdline);

    if (!suite)
	    return NULL;

    if (!add_tests(suite))
        return NULL;

    return suite;
}

// eof
