/* $Id: dict.c,v 1.1.1.1 2010/11/27 21:23:59 agc Exp $ */

/* Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mateusz Kocielski.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.	IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/queue.h>
#include "dict.h"

/* local headers */

/** dictionary */
LIST_HEAD(saslc__dict_t, saslc__dict_node_t);

/** linked list */
typedef struct saslc__dict_node_t {
	LIST_ENTRY(saslc__dict_node_t) nodes;
	char *key; /**< key */
	char *value; /**< value */
	size_t value_len; /**< value length */
} saslc__dict_node_t;

static bool saslc__valid_key(const char *);
static void saslc__list_node_destroy(saslc__dict_node_t *);
static saslc__dict_node_t *saslc__get_node_by_key(saslc__dict_t *,
    const char *);

/**
 * @brief checks if the key is legal
 * @param key node key
 * @return true if key is legal, false otherwise
 */

static bool
saslc__valid_key(const char *key)
{
	size_t i; /* index */

	for (i = 0; key[i]; i++) {
		if (!isalnum((unsigned char)key[i]))
			return false;
	}

	return true;
}

/**
 * @brief destroys and deallocates list node
 * @param node list node
 */

static void
saslc__list_node_destroy(saslc__dict_node_t *node)
{
	free(node->key);
	/* zero value, it may contain sensitive data */
	memset(node->value, 0, node->value_len);
	free(node->value);
	LIST_REMOVE(node, nodes);
	free(node);
}

/**
 * @brief gets node from the dictionary using key
 * @param dict dictionary
 * @param key node key
 * @return pointer to node if key is in the dictionary, NULL otherwise
 */

static saslc__dict_node_t *
saslc__get_node_by_key(saslc__dict_t *dict, const char *key)
{
	saslc__dict_node_t *node;
	for (node = dict->lh_first; node != NULL; node = node->nodes.le_next) {
		if (strcmp(node->key, key) == 0)
			return node;
	}

	return NULL;
}

/**
 * @brief destroys and deallocates dictionary
 * @param dict dictionary
 */

void
saslc__dict_destroy(saslc__dict_t *dict)
{
	while(dict->lh_first != NULL)
		saslc__list_node_destroy(dict->lh_first);

	free(dict);
}

/**
 * @brief removes node from the dictionary using key
 * @param dict dictionary
 * @param key node key
 * @return DICT_OK on success, DICT_KEYNOTFOUND if node was not found (key
 * does not exist in the dictionary.
 */

int
saslc__dict_remove(saslc__dict_t *dict, const char *key)
{
	saslc__dict_node_t *node;

	for (node = dict->lh_first; node != NULL;
	    node = node->nodes.le_next) {
		if (strcmp(node->key, key) == 0) {
			saslc__list_node_destroy(node);
			return DICT_OK;
		}
	}

	return DICT_KEYNOTFOUND;
}

/**
 * @brief gets node value from the dictionary using key
 * @param dict dictionary
 * @param key node key
 * @return pointer to the value if key was found in the dictionary, NULL
 * otherwise.
 */

const char *
saslc__dict_get(saslc__dict_t *dict, const char *key)
{
	saslc__dict_node_t *node = saslc__get_node_by_key(dict, key);

	return (node != NULL) ? node->value : NULL;
}

/**
 * @brief gets length of node value from the dictionary using key
 * @param dict dictionary
 * @param key node key
 * @return length of the node value, 0 is returned in case when key does not
 * exist in the dictionary.
 */

size_t
saslc__dict_get_len(saslc__dict_t *dict, const char *key)
{
	saslc__dict_node_t *node = saslc__get_node_by_key(dict, key);

	return (node != NULL) ? node->value_len : 0;
}

/**
 * @brief creates and allocates dictionary
 * @return pointer to new dictionary, NULL is returned on allocation failure
 */

saslc__dict_t *
saslc__dict_create(void)
{
	saslc__dict_t *head;

	head = calloc(1, sizeof(saslc__dict_t));

	if (head == NULL)
		return NULL;

	LIST_INIT(head);

	return head;
}

/**
 * @brief inserts node into dictionary
 * @param dict dictionary
 * @param key node key
 * @param val node value
 * @return
 * DICT_OK - on success,
 * DICT_KEYINVALID - if node key is illegal,
 * DICT_VALBAD - if node value is illegal,
 * DICT_KEYEXISTS - if node with the same key already exists in the
 * dictionary,
 * DICT_NOMEM - on allocation failure
 */

int
saslc__dict_insert(saslc__dict_t *dict, const char *key, const char *val)
{
	char *d_key, *d_val;
	saslc__dict_node_t *node;

	if (key == NULL || saslc__valid_key(key) == false) 
		return DICT_KEYINVALID;

	if (val == NULL)
		return DICT_VALBAD;

	/* check if key exists in dictionary */
	if (saslc__dict_get(dict, key) != NULL)
		return DICT_KEYEXISTS;

	if ((d_key = strdup(key)) == NULL)
		return DICT_NOMEM;

	if ((d_val = strdup(val)) == NULL) {
		free(d_key);
		return DICT_NOMEM;
	}

	if ((node = calloc(1, sizeof(*node))) == NULL) {
		free(d_val);
		free(d_key);
		return DICT_NOMEM;
	}

	LIST_INSERT_HEAD(dict, node, nodes);
	node->key = d_key;
	node->value = d_val;
	node->value_len = strlen(node->value);

	return DICT_OK;
}
