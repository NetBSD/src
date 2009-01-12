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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <sys/stat.h>

#include "CUnit/Basic.h"
#include "openpgpsdk/readerwriter.h"

#include "tests.h"

int main()
    {

    setup();

    if (CUE_SUCCESS != CU_initialize_registry())
        {
        fprintf(stderr,"ERROR: initializing registry\n");
        return CU_get_error();
        }

    if (NULL == suite_crypto())
        {
        fprintf(stderr,"ERROR: initialising suite_crypto\n");
        CU_cleanup_registry();
        return CU_get_error();
        }

    if (NULL == suite_packet_types())
        {
        fprintf(stderr,"ERROR: initialising suite_packet_types\n");
        CU_cleanup_registry();
        return CU_get_error();
        }

    if (NULL == suite_rsa_encrypt()) 
        {
        fprintf(stderr,"ERROR: initialising suite_rsa_encrypt\n");
        CU_cleanup_registry();
        return CU_get_error();
        }
    if (NULL == suite_rsa_decrypt()) 
        {
        fprintf(stderr,"ERROR: initialising suite_rsa_decrypt\n");
        CU_cleanup_registry();
        return CU_get_error();
        }

    if (NULL == suite_rsa_signature()) 
        {
        fprintf(stderr,"ERROR: initialising suite_rsa_signature\n");
        CU_cleanup_registry();
        return CU_get_error();
        }

    if (NULL == suite_rsa_verify()) 
        {
        fprintf(stderr,"ERROR: initialising suite_rsa_verify\n");
        CU_cleanup_registry();
        return CU_get_error();
        }

    if (NULL == suite_rsa_keys())
        {
        fprintf(stderr,"ERROR: initialising suite_rsa_keys\n");
        CU_cleanup_registry();
        return CU_get_error();
        }

    if (NULL == suite_dsa_signature()) 
        {
        fprintf(stderr,"ERROR: initialising suite_dsa_signature\n");
        CU_cleanup_registry();
        return CU_get_error();
        }

    if (NULL == suite_dsa_verify()) 
        {
        fprintf(stderr,"ERROR: initialising suite_dsa_verify\n");
        CU_cleanup_registry();
        return CU_get_error();
        }

    if (NULL == suite_cmdline())
        {
        fprintf(stderr,"ERROR: initialising suite_cmdline\n");
        CU_cleanup_registry();
        return CU_get_error();
        }

    // Run tests
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    cleanup();
    CU_cleanup_registry();

    return CU_get_error();
    }

// EOF
