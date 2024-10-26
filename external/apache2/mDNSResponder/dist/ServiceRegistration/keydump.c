/* keydump.c
 *
 * Copyright (c) 2018 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Dump the contents of a key file saved by e.g. srp-simple as a DNS key.
 */

#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "srp-api.h"

int
main(int argc, char **argv)
{
    const char *key_name = "com.apple.srp-client.host-key";
    srp_key_t *key;

    key = srp_get_key(key_name, NULL);
    if (key == NULL) {
        if (key == NULL) {
            printf("Unable to load key from %s.", key_name);
            exit(1);
        }
    }

    srp_print_key(key);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
