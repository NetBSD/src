/*
 * util/edns.h - handle base EDNS options.
 *
 * Copyright (c) 2018, NLnet Labs. All rights reserved.
 *
 * This software is open source.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of the NLNET LABS nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *
 * This file contains functions for base EDNS options.
 */

#ifndef UTIL_EDNS_H
#define UTIL_EDNS_H

#include "util/storage/dnstree.h"

struct edns_data;
struct config_file;
struct comm_point;
struct regional;

/**
 * Structure containing all EDNS strings.
 */
struct edns_strings {
	/** Tree of EDNS client strings to use in upstream queries, per address
	 * prefix. Contains nodes of type edns_string_addr. */
	rbtree_type client_strings;
	/** EDNS opcode to use for client strings */
	uint16_t client_string_opcode;
	/** region to allocate tree nodes in */
	struct regional* region;
};

/**
 * EDNS string. Node of rbtree, containing string and prefix.
 */
struct edns_string_addr {
	/** node in address tree, used for tree lookups. Need to be the first
	 * member of this struct. */
	struct addr_tree_node node;
	/** string, ascii format */
	uint8_t* string;
	/** length of string */
	size_t string_len;
};

/**
 * Create structure to hold EDNS strings
 * @return: newly created edns_strings, NULL on alloc failure.
 */
struct edns_strings* edns_strings_create(void);

/** Delete EDNS strings structure
 * @param edns_strings: struct to delete
 */
void edns_strings_delete(struct edns_strings* edns_strings);

/**
 * Add configured EDNS strings
 * @param edns_strings: edns strings to apply config to
 * @param config: struct containing EDNS strings configuration
 * @return 0 on error
 */
int edns_strings_apply_cfg(struct edns_strings* edns_strings,
	struct config_file* config);

/**
 * Find string for address.
 * @param tree: tree containing EDNS strings per address prefix.
 * @param addr: address to use for tree lookup
 * @param addrlen: length of address
 * @return: matching tree node, NULL otherwise
 */
struct edns_string_addr*
edns_string_addr_lookup(rbtree_type* tree, struct sockaddr_storage* addr,
	socklen_t addrlen);

#endif
