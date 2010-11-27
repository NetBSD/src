/* $Id: parser.c,v 1.1.1.1 2010/11/27 21:23:59 agc Exp $ */

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
 *  	  This product includes software developed by the NetBSD
 *  	  Foundation, Inc. and its contributors.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <err.h>
#include <saslc.h>
#include "saslc_private.h"
#include "dict.h"
#include "parser.h"

/* local headers */

/* token types */
enum {
	TOKEN_OPTION,	/* option */
	TOKEN_STRING,	/* quoted string */
	TOKEN_NUM,	/* number */
	TOKEN_UNKNOWN	/* unknown */
};

/** token structure */
typedef struct saslc__token_t {
	int type; /**< token type */
	char *val; /**< token value */
	struct saslc__token_t *next; /**< next token */
} saslc__token_t;


static void saslc__destroy_tokens(saslc__token_t *);
static saslc__token_t *saslc__get_token(char **);
static saslc__token_t *saslc__get_tokens(char *);
static int saslc__parse_line(char *, saslc__dict_t *); 
static int saslc__parse_file(saslc_t *, char *, saslc__dict_t *);
 
/**
 * @brief destroys tokens list and freeing resouces.
 * @param token token
 */

static void
saslc__destroy_tokens(saslc__token_t *token)
{
	saslc__token_t *next;

	while(token != NULL) {
		next = token->next;
		free(token);
		token = next;
	}

	return;
}

/**
 * @brief gets token from string c and updates pointer position.
 * @param c pointer to string
 * @return token on success, NULL on failure (e.g. at end of string). Note that
 * c is updated to point on next token.
 */

static saslc__token_t *
saslc__get_token(char **c)
{
	saslc__token_t *token;

	/* omit spaces and tabs */
	while (**c == ' ' || **c == '\t')
		(*c)++;

	if (**c == '\0')
		return NULL;

	if ((token = calloc(1, sizeof(saslc__token_t))) == NULL)
		return NULL;

	token->val = *c;
	token->type = TOKEN_UNKNOWN;

	/* try to recognize type of the token by prefix */
	if (**c == '\"')
		token->type = TOKEN_STRING;

	if (isdigit((unsigned char)**c))
		token->type = TOKEN_NUM;

	if (isalpha((unsigned char)**c))
		token->type = TOKEN_OPTION;

	/* move to the next unchecked character */
	(*c)++;

	switch(token->type)
	{
	case TOKEN_NUM:
		while (isdigit((unsigned char)**c))
			(*c)++;
		break;
	case TOKEN_OPTION:
		while (isalnum((unsigned char)**c))
			(*c)++;
		break;
	case TOKEN_STRING:
		while (**c != '\0' && **c != '\"')
			(*c)++;
		if (**c == '\"') {
			(*c)++;
			break;
		}
	/*FALLTHROUGH*/
	default:
		/* UNKNOWN TOKEN */
		free(token);
		return NULL;
	}

	if (isspace((unsigned char)**c)) {
		**c = '\0';
		(*c)++;
	}

	return token;
}

/**
 * @brief tokenizes line.
 * @param line input line
 * @return list of tokens, NULL on error
 */

static saslc__token_t *
saslc__get_tokens(char *line)
{
	saslc__token_t *act, *prev;
	saslc__token_t *head;
	char *c;

	c = line;

	for (head = NULL, prev = NULL, act = saslc__get_token(&c);
	    act != NULL;) {
		/* set head */
		if (head == NULL)
			head = act;
		
		/* remember previous node */
		if (prev != NULL)
			prev->next = act;

		/* get next token */
		prev = act;
		act = saslc__get_token(&c);

		/* an error occured */
		if (act == NULL && *c != '\0') {
			/* free list */
		    	saslc__destroy_tokens(head);
			return NULL;
		}
	}

	return head;
}

/**
 * @brief parses line and store result in dict.
 * @param line input line
 * @param dict dictionary in which parsed options will be stored
 * @return 0 on success, -1 on failure.
 */

static int
saslc__parse_line(char *line, saslc__dict_t *dict)
{
	char *opt, *val, len;
	int rv = -1;
	saslc__token_t *token, *head;

	token = saslc__get_tokens(line);

	/* line can't be parsed */
	if (token == NULL)
		return -1;

	head = token; /* keep pointer to head (to free memory at the end) */

	while (token != NULL) {
		if (token->type != TOKEN_OPTION)
		goto out;
		/* get option */
		opt = token->val;
		token = token->next;
		/* check if value is specified */
		if (token == NULL)
			goto out;
		val = token->val;
		if (token->type == TOKEN_STRING) {
			/* striping " */
			val++;
			len = strlen(val);
			if (len == 0)
				goto out;
			val[len-1] = '\0';
		}
		/* check if value has got proper type */
		if (token->type != TOKEN_STRING && token->type !=
		    TOKEN_NUM)
			goto out;
		/* insert (option, value) into dictionary */
		if (saslc__dict_insert(dict, opt, val) < 0)
			goto out;
		token = token->next; /* parse next token */
	}
	rv = 0;

out:
	saslc__destroy_tokens(head);

	return rv;
}

/**
 * @brief parses file and store result in dict
 * @param ctx saslc context
 * @param path path to the file
 * @param dict dictionary in which parsed options will be stored
 * @return 0 on success, -1 on failure.
 */

static int
saslc__parse_file(saslc_t *ctx, char *path, saslc__dict_t *dict)
{
	char input[LINE_MAX], *c;
	FILE *fd;
	int rv = 0;

	fd = fopen(path, "r");

	if (fd == NULL) {
		/* XXX */
		saslc__error_set(ERR(ctx), ERROR_PARSE, "can't open file");
		return 0;
	}

	while (fgets(input, sizeof(input), fd) != NULL) {
		/* strip newline char */
		c = strchr(input, '\n');
		if (c != NULL)
			*c = '\0';
		
		if (feof(fd) > 0)
			break;

		/* parse line */
		if (saslc__parse_line(input, dict) < 0) {
			/* XXX */
			saslc__error_set(ERR(ctx), ERROR_PARSE,
			    "can't parse file");
			rv = -1;
			break;
		}
	}

	fclose(fd);
	return rv;
}

/**
 * @brief parse configuration files. By default function reads
 * files from /etc/saslc.d/saslc/ directory if appname is not setup. Otherwise
 * function uses /etc/saslc.d/[appname]/ directory. /etc/saslc.d/ is default
 * directory which stores configuration for all applications, but can be
 * overwritten by SASLC_CONFIG variable in environment.
 * @param ctx saslc context
 * @return 0 on success, -1 on failure.
 */

int
saslc__parser_config(saslc_t *ctx)
{
	char path[FILENAME_MAX];
	const char *config_path;
	const char *appname;
	saslc__mech_list_node_t *mech_node;


	if ((config_path = getenv(SASLC__ENV_PATH)) == NULL)
		config_path = SASLC__CONFIG_PATH;

	if ((appname = ctx->appname) == NULL)
		appname = SASLC__DEFAULT_APPNAME;

	/* parse general */
	snprintf(path, sizeof(path), "%s/%s/%s%s", config_path,
	    appname,SASLC__CONFIG_MAIN_FILE, SASLC__CONFIG_SUFFIX);
	if (saslc__parse_file(ctx, path, ctx->prop) < 0)
		return -1;
	
	/* parse mechs */
	for (mech_node = ctx->mechanisms->lh_first; mech_node != NULL;
	    mech_node = mech_node->nodes.le_next) {
		snprintf(path, sizeof(path), "%s/%s/%s/%s%s",
		    config_path, appname, SASLC__CONFIG_MECH_DIRECTORY,
		    mech_node->mech->name, SASLC__CONFIG_SUFFIX);
		if (saslc__parse_file(ctx, path, mech_node->prop) < 0)
			return -1;
	}

	return 0;
}
