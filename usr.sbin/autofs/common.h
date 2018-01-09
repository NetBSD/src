/*	$NetBSD: common.h,v 1.1 2018/01/09 03:31:15 christos Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * Copyright (c) 2016 The DragonFly Project
 * Copyright (c) 2014 The FreeBSD Foundation
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tomohiro Kusumi <kusumi.tomohiro@gmail.com>.
 *
 * This software was developed by Edward Tomasz Napierala under sponsorship
 * from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef AUTOMOUNTD_H
#define	AUTOMOUNTD_H

#include <sys/types.h>
#include <sys/queue.h>
#include <stdio.h>
#include <stdbool.h>

#define	AUTO_MASTER_PATH	"/etc/auto_master"
#define	AUTO_MAP_PREFIX		"/etc"
#define	AUTO_SPECIAL_PREFIX	"/etc/autofs"
#define	AUTO_INCLUDE_PATH	AUTO_SPECIAL_PREFIX "/include"

struct node {
	TAILQ_ENTRY(node)	n_next;
	TAILQ_HEAD(nodehead, node)	n_children;
	struct node		*n_parent;
	char			*n_key;
	char			*n_options;
	char			*n_location;
	char			*n_map;
	const char		*n_config_file;
	int			n_config_line;
};

struct defined_value {
	TAILQ_ENTRY(defined_value)	d_next;
	char				*d_name;
	char				*d_value;
};

void	log_init(int);
void	log_set_peer_name(const char *);
void	log_set_peer_addr(const char *);
void	log_err(int, const char *, ...) __printflike(2, 3);
void	log_errx(int, const char *, ...) __printflike(2, 3);
void	log_warn(const char *, ...) __printflike(1, 2);
void	log_warnx(const char *, ...) __printflike(1, 2);
void	log_debugx(const char *, ...) __printflike(1, 2);

char	*checked_strdup(const char *);
char	*concat(const char *, char, const char *);
void	create_directory(const char *);

struct node *node_new_root(void);
struct node *node_new(struct node *, char *, char *, char *, const char *,
    int);
struct node *node_new_map(struct node *, char *, char *, char *,
    const char *, int);
struct node *node_find(struct node *, const char *mountpoint);
bool	node_is_direct_map(const struct node *);
bool	node_has_wildcards(const struct node *);
char	*node_path(const struct node *);
char	*node_options(const struct node *);
void	node_expand_ampersand(struct node *, const char *);
void	node_expand_wildcard(struct node *, const char *);
int	node_expand_defined(struct node *);
void	node_expand_indirect_maps(struct node *);
void	node_print(const struct node *, const char *);
void	parse_master(struct node *, const char *);
void	parse_map(struct node *, const char *, const char *, bool *);
char	*defined_expand(const char *);
void	defined_init(void);
void	defined_parse_and_add(char *);
void	lesser_daemon(void);

int	main_automount(int, char **);
int	main_automountd(int, char **);
int	main_autounmountd(int, char **);

FILE	*auto_popen(const char *, ...);
int	auto_pclose(FILE *);

/*
 * lex(1) stuff.
 */
extern int lineno;

#define	STR	1
#define	NEWLINE	2

#endif /* !AUTOMOUNTD_H */
