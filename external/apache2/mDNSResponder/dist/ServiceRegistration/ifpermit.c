/* ifpermit.c
 *
 * Copyright (c) 2024 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Implementation of a permitted interface list object, which maintains a list of
 * interfaces on which we are permitted to provide some service.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <dns_sd.h>
#include <net/if.h>
#include <inttypes.h>
#include <sys/resource.h>
#include <netinet/icmp6.h>
#include "srp.h"
#include "ifpermit.h"
#include "dns-msg.h"
#include "ioloop.h"
#include "srp-mdns-proxy.h"

// If we aren't able to allocate a permitted interface list, we still need to return a non-NULL value so that we don't
// fail open. So all of these functions need to treat this particular value as special but not dereference it.
#define PERMITTED_INTERFACE_LIST_BLOCKED (ifpermit_list_t *)1

typedef struct ifpermit_name ifpermit_name_t;
struct ifpermit_name {
    ifpermit_name_t *next;
    char *name; // Interface name
    uint32_t ifindex; // Interface index
    int count;        // Number of permittors for this interface
};

struct ifpermit_list {
	int ref_count;
    ifpermit_name_t *names;
};

void
ifpermit_list_add(ifpermit_list_t *permits, const char *name)
{
	if (permits == PERMITTED_INTERFACE_LIST_BLOCKED) {
		ERROR("blocked permit list when adding " PUB_S_SRP, name);
        return;
	}
    ifpermit_name_t **pname = &permits->names;
    ifpermit_name_t *permit_name;
    while (*pname != NULL) {
        permit_name = *pname;
        if (!strcmp(name, permit_name->name)) {
        success:
            permit_name->count++;
            INFO("%d permits for interface " PUB_S_SRP " with index %d", permit_name->count, name, permit_name->ifindex);
            return;
        }
        pname = &permit_name->next;
    }
    permit_name = calloc(1, sizeof(*permit_name));
    if (permit_name != NULL) {
        permit_name->name = strdup(name);
        if (permit_name->name == NULL) {
            free(permit_name);
            permit_name = NULL;
        } else {
            permit_name->ifindex = if_nametoindex(name);
            if (permit_name->ifindex == 0) {
                ERROR("if_nametoindex for interface " PUB_S_SRP " returned 0.", name);
                free(permit_name->name);
                free(permit_name);
                return;
            }
            *pname = permit_name;
            goto success;
        }
    }
    ERROR("no memory to add permit for " PUB_S_SRP, name);
}

void
ifpermit_list_remove(ifpermit_list_t *permits, const char *name)
{
	if (permits == PERMITTED_INTERFACE_LIST_BLOCKED) {
		ERROR("blocked permit list when removing " PUB_S_SRP, name);
        return;
    }
    ifpermit_name_t **pname = &permits->names;
    ifpermit_name_t *permit_name;
    while (*pname != NULL) {
        permit_name = *pname;
        if (!strcmp(name, permit_name->name)) {
            permit_name->count--;
            INFO("%d permits for interface " PUB_S_SRP " with index %d", permit_name->count, name, permit_name->ifindex);
            if (permit_name->count == 0) {
                *pname = permit_name->next;
                free(permit_name->name);
                free(permit_name);
            }
            return;
        }
        pname = &permit_name->next;
    }

    FAULT("permit remove for interface " PUB_S_SRP " which does not exist", name);
}

static void
ifpermit_list_finalize(ifpermit_list_t *list)
{
    if (list != NULL && list != PERMITTED_INTERFACE_LIST_BLOCKED) {
        ifpermit_name_t *names = list->names, *next = NULL;
        while (names != NULL) {
            next = names->next;
            free(names->name);
            free(names);
            names = next;
        }
        free(list);
    }
}

void
ifpermit_list_retain_(ifpermit_list_t *list, const char *file, int line)
{
    if (list != NULL && list != PERMITTED_INTERFACE_LIST_BLOCKED) {
        RETAIN(list, ifpermit_list);
    }
}

void
ifpermit_list_release_(ifpermit_list_t *list, const char *file, int line)
{
    if (list != NULL && list != PERMITTED_INTERFACE_LIST_BLOCKED) {
        RELEASE(list, ifpermit_list);
    }
}

ifpermit_list_t *
ifpermit_list_create_(const char *file, int line)
{
    ifpermit_list_t *permits = calloc(1, sizeof(*permits));
    if (permits == NULL) {
        return PERMITTED_INTERFACE_LIST_BLOCKED;
    }
    RETAIN(permits, ifpermit_list);
    return permits;
}

bool
ifpermit_interface_is_permitted(ifpermit_list_t *permits, uint32_t ifindex)
{
    if (permits != NULL && permits != PERMITTED_INTERFACE_LIST_BLOCKED) {
        for (ifpermit_name_t *name = permits->names; name != NULL; name = name->next) {
            if (name->ifindex == ifindex) {
                return true;
            }
        }
    }
    return false;
}

void ifpermit_add_permitted_interface_to_server_(srp_server_t *NONNULL server_state, const char *NONNULL name,
                                                 const char *file, int line)
{
    if (server_state->permitted_interfaces == NULL) {
        server_state->permitted_interfaces = ifpermit_list_create_(file, line);
    }
    ifpermit_list_add(server_state->permitted_interfaces, name);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
