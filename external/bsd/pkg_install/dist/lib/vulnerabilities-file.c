/*-
 * Copyright (c) 2008 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <nbcompat.h>

#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
__RCSID("$NetBSD: vulnerabilities-file.c,v 1.1.1.1 2008/09/30 19:00:27 joerg Exp $");

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <ctype.h>
#if HAVE_ERR_H
#include <err.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#ifndef NETBSD
#include <nbcompat/sha1.h>
#include <nbcompat/sha2.h>
#else
#include <sha1.h>
#include <sha2.h>
#endif
#include <unistd.h>

#include "lib.h"

/*
 * We explicitely initialize this to NULL to stop Mac OS X Leopard's linker
 * from turning this into a common symbol which causes a link failure.
 */
const char *gpg_cmd = NULL;

static void
verify_signature(const char *input, size_t input_len)
{
	pid_t child;
	int fd[2], status;

	if (gpg_cmd == NULL)
		errx(EXIT_FAILURE, "GPG variable not set in configuration file");

	if (pipe(fd) == -1)
		err(EXIT_FAILURE, "cannot create input pipes");

	child = vfork();
	if (child == -1)
		err(EXIT_FAILURE, "cannot fork GPG process");
	if (child == 0) {
		close(fd[1]);
		close(STDIN_FILENO);
		if (dup2(fd[0], STDIN_FILENO) == -1) {
			static const char err_msg[] =
			    "cannot redirect stdin of GPG process\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
			_exit(255);
		}
		close(fd[0]);
		execlp(gpg_cmd, gpg_cmd, "--verify", "-", (char *)NULL);
		_exit(255);
	}
	close(fd[0]);
	if (write(fd[1], input, input_len) != input_len)
		errx(EXIT_FAILURE, "Short read from GPG");
	close(fd[1]);
	waitpid(child, &status, 0);
	if (status)
		errx(EXIT_FAILURE, "GPG could not verify the signature");
}

static void *
sha512_hash_init(void)
{
	static SHA512_CTX hash_ctx;

	SHA512_Init(&hash_ctx);
	return &hash_ctx;
}

static void
sha512_hash_update(void *ctx, const void *data, size_t len)
{
	SHA512_CTX *hash_ctx = ctx;

	SHA512_Update(hash_ctx, data, len);
}

static const char *
sha512_hash_finish(void *ctx)
{
	static char hash[SHA512_DIGEST_STRING_LENGTH];
	SHA512_CTX *hash_ctx = ctx;

	SHA512_End(hash_ctx, hash);

	return hash;
}

static void *
sha1_hash_init(void)
{
	static SHA1_CTX hash_ctx;

	SHA1Init(&hash_ctx);
	return &hash_ctx;
}

static void
sha1_hash_update(void *ctx, const void *data, size_t len)
{
	SHA1_CTX *hash_ctx = ctx;

	SHA1Update(hash_ctx, data, len);
}

static const char *
sha1_hash_finish(void *ctx)
{
	static char hash[SHA1_DIGEST_STRING_LENGTH];
	SHA1_CTX *hash_ctx = ctx;

	SHA1End(hash_ctx, hash);

	return hash;
}

static const struct hash_algorithm {
	const char *name;
	size_t name_len;
	void * (*init)(void);
	void (*update)(void *, const void *, size_t);
	const char * (* finish)(void *);
} hash_algorithms[] = {
	{ "SHA512", 6, sha512_hash_init, sha512_hash_update,
	  sha512_hash_finish },
	{ "SHA1", 4, sha1_hash_init, sha1_hash_update,
	  sha1_hash_finish },
	{ NULL, 0, NULL, NULL, NULL }
};

static void
verify_hash(const char *input, const char *hash_line)
{
	const struct hash_algorithm *hash;
	void *ctx;
	const char *last_start, *next, *hash_value;

	for (hash = hash_algorithms; hash->name != NULL; ++hash) {
		if (strncmp(hash_line, hash->name, hash->name_len))
			continue;
		if (isspace((unsigned char)hash_line[hash->name_len]))
			break;
	}
	if (hash->name == NULL) {
		const char *end_name;
		for (end_name = hash_line; *end_name != '\0'; ++end_name) {
			if (!isalnum((unsigned char)*end_name))
				break;
		}
		warnx("Unsupported hash algorithm: %.*s",
		    (int)(end_name - hash_line), hash_line); 
		return;
	}

	hash_line += hash->name_len;
	if (!isspace((unsigned char)*hash_line))
		errx(EXIT_FAILURE, "Invalid #CHECKSUM");
	while (isspace((unsigned char)*hash_line) && *hash_line != '\n')
		++hash_line;

	if (*hash_line == '\n')
		errx(EXIT_FAILURE, "Invalid #CHECKSUM");

	ctx = (*hash->init)();
	for (last_start = input; *input != '\0'; input = next) {
		if ((next = strchr(input, '\n')) == NULL)
			errx(EXIT_FAILURE, "Missing newline in pkg-vulnerabilities");
		++next;
		if (*input == '\n' ||
		    strncmp(input, "-----BEGIN", 10) == 0 ||
		    strncmp(input, "Hash:", 5) == 0 ||
		    strncmp(input, "# $NetBSD", 9) == 0 ||
		    strncmp(input, "#CHECKSUM", 9) == 0) {
			(*hash->update)(ctx, last_start, input - last_start);
			last_start = next;
		} else if (strncmp(input, "Version:", 8) == 0)
			break;
	}
	(*hash->update)(ctx, last_start, input - last_start);
	hash_value = (*hash->finish)(ctx);
	if (strncmp(hash_line, hash_value, strlen(hash_value)))
		errx(EXIT_FAILURE, "%s hash doesn't match", hash->name);
	hash_line += strlen(hash_value);

	while (isspace((unsigned char)*hash_line) && *hash_line != '\n')
		++hash_line;
	
	if (!isspace((unsigned char)*hash_line))
		errx(EXIT_FAILURE, "Invalid #CHECKSUM");
}

static void
add_vulnerability(struct pkg_vulnerabilities *pv, size_t *allocated, const char *line)
{
	size_t len_pattern, len_class, len_url;
	const char *start_pattern, *start_class, *start_url;

	start_pattern = line;

	start_class = line;
	while (*start_class != '\0' && !isspace((unsigned char)*start_class))
		++start_class;
	len_pattern = start_class - line;

	while (*start_class != '\n' && isspace((unsigned char)*start_class))
		++start_class;

	if (*start_class == '0' || *start_class == '\n')
		errx(EXIT_FAILURE, "Input error: missing classification");

	start_url = start_class;
	while (*start_url != '\0' && !isspace((unsigned char)*start_url))
		++start_url;
	len_class = start_url - start_class;

	while (*start_url != '\n' && isspace((unsigned char)*start_url))
		++start_url;

	if (*start_url == '0' || *start_url == '\n')
		errx(EXIT_FAILURE, "Input error: missing URL");

	line = start_url;
	while (*line != '\0' && !isspace((unsigned char)*line))
		++line;
	len_url = line - start_url;

	if (pv->entries == *allocated) {
		if (*allocated == 0)
			*allocated = 16;
		else if (*allocated <= SSIZE_MAX / 2)
			*allocated *= 2;
		else
			errx(EXIT_FAILURE, "Too many vulnerabilities");
		pv->vulnerability = realloc(pv->vulnerability,
		    sizeof(char *) * *allocated);
		pv->classification = realloc(pv->classification,
		    sizeof(char *) * *allocated);
		pv->advisory = realloc(pv->advisory,
		    sizeof(char *) * *allocated);
		if (pv->vulnerability == NULL ||
		    pv->classification == NULL || pv->advisory == NULL)
			errx(EXIT_FAILURE, "realloc failed");
	}

	if ((pv->vulnerability[pv->entries] = malloc(len_pattern + 1)) == NULL)
		errx(EXIT_FAILURE, "malloc failed");
	memcpy(pv->vulnerability[pv->entries], start_pattern, len_pattern);
	pv->vulnerability[pv->entries][len_pattern] = '\0';
	if ((pv->classification[pv->entries] = malloc(len_class + 1)) == NULL)
		errx(EXIT_FAILURE, "malloc failed");
	memcpy(pv->classification[pv->entries], start_class, len_class);
	pv->classification[pv->entries][len_class] = '\0';
	if ((pv->advisory[pv->entries] = malloc(len_url + 1)) == NULL)
		errx(EXIT_FAILURE, "malloc failed");
	memcpy(pv->advisory[pv->entries], start_url, len_url);
	pv->advisory[pv->entries][len_url] = '\0';

	++pv->entries;
}

struct pkg_vulnerabilities *
read_pkg_vulnerabilities(const char *path, int ignore_missing, int check_sum)
{
	struct pkg_vulnerabilities *pv;
	struct stat st;
	int fd;
	char *input, *decompressed_input;
	size_t input_len, decompressed_len;
	ssize_t bytes_read;

	if ((fd = open(path, O_RDONLY)) == -1) {
		if (errno == ENOENT && ignore_missing)
			return NULL;
		err(EXIT_FAILURE, "Cannot open %s", path);
	}

	if (fstat(fd, &st) == -1)
		err(EXIT_FAILURE, "Cannot stat %s", path);

	if ((st.st_mode & S_IFMT) != S_IFREG)
		errx(EXIT_FAILURE, "Input is not regular file");
	if (st.st_size > SSIZE_MAX - 1)
		errx(EXIT_FAILURE, "Input too large");

	input_len = (size_t)st.st_size;
	if (input_len < 4)
		err(EXIT_FAILURE, "Input too short for a pkg_vulnerability file");
	if ((input = malloc(input_len + 1)) == NULL)
		err(EXIT_FAILURE, "malloc failed");
	if ((bytes_read = read(fd, input, input_len)) == -1)
		err(1, "Failed to read input");
	if (bytes_read != st.st_size)
		errx(1, "Unexpected short read");

	if (decompress_buffer(input, input_len, &decompressed_input,
	    &decompressed_len)) {
		free(input);
		input = decompressed_input;
		input_len = decompressed_len;
	}
	pv = parse_pkg_vulnerabilities(input, input_len, check_sum);
	free(input);

	return pv;
}

struct pkg_vulnerabilities *
parse_pkg_vulnerabilities(const char *input, size_t input_len, int check_sum)
{
	struct pkg_vulnerabilities *pv;
	long version;
	char *end;
	const char *iter, *next;
	size_t allocated_vulns;

	pv = malloc(sizeof(*pv));
	if (pv == NULL)
		err(EXIT_FAILURE, "malloc failed");

	allocated_vulns = pv->entries = 0;
	pv->vulnerability = NULL;
	pv->classification = NULL;
	pv->advisory = NULL;

	if (strlen(input) != input_len)
		errx(1, "Invalid input (NUL character found)");

	if (check_sum)
		verify_signature(input, input_len);

	for (iter = input; *iter; iter = next) {
		if ((next = strchr(iter, '\n')) == NULL)
			errx(EXIT_FAILURE, "Missing newline in pkg-vulnerabilities");
		++next;
		if (*iter == '\0' || *iter == '\n')
			continue;
		if (strncmp(iter, "-----BEGIN", 10) == 0)
			continue;
		if (strncmp(iter, "Hash:", 5) == 0)
			continue;
		if (strncmp(iter, "# $NetBSD", 9) == 0)
			continue;
		if (*iter == '#' && isspace((unsigned char)iter[1])) {
			for (++iter; iter != next; ++iter) {
				if (!isspace((unsigned char)*iter))
					errx(EXIT_FAILURE, "Invalid header");
			}
			continue;
		}

		if (strncmp(iter, "#FORMAT", 7) != 0)
			errx(EXIT_FAILURE, "Input header is malformed");

		iter += 7;
		if (!isspace((unsigned char)*iter))
			errx(EXIT_FAILURE, "Invalid #FORMAT");
		++iter;
		version = strtol(iter, &end, 10);
		if (iter == end || version != 1 || *end != '.')
			errx(EXIT_FAILURE, "Input #FORMAT");
		iter = end + 1;
		version = strtol(iter, &end, 10);
		if (iter == end || version != 1 || *end != '.')
			errx(EXIT_FAILURE, "Input #FORMAT");
		iter = end + 1;
		version = strtol(iter, &end, 10);
		if (iter == end || version != 0)
			errx(EXIT_FAILURE, "Input #FORMAT");
		for (iter = end; iter != next; ++iter) {
			if (!isspace((unsigned char)*iter))
				errx(EXIT_FAILURE, "Input #FORMAT");
		}
		break;
	}
	if (*iter == '\0')
		errx(EXIT_FAILURE, "Missing #CHECKSUM or content");

	for (iter = next; *iter; iter = next) {
		if ((next = strchr(iter, '\n')) == NULL)
			errx(EXIT_FAILURE, "Missing newline in pkg-vulnerabilities");
		++next;
		if (*iter == '\0' || *iter == '\n')
			continue;
		if (strncmp(iter, "Version:", 5) == 0)
			break;
		if (*iter == '#' &&
		    (iter[1] == '\0' || iter[1] == '\n' || isspace((unsigned char)iter[1])))
			continue;
		if (strncmp(iter, "#CHECKSUM", 9) == 0) {
			iter += 9;
			if (!isspace((unsigned char)*iter))
				errx(EXIT_FAILURE, "Invalid #CHECKSUM");
			while (isspace((unsigned char)*iter))
				++iter;
			verify_hash(input, iter);
			continue;
		}
		if (*iter == '#') {
			/*
			 * This should really be an error,
			 * but it is still used.
			 */
			/* errx(EXIT_FAILURE, "Invalid data line starting with #"); */
			continue;
		}
		add_vulnerability(pv, &allocated_vulns, iter);
	}

	if (pv->entries != allocated_vulns) {
		pv->vulnerability = realloc(pv->vulnerability,
		    sizeof(char *) * pv->entries);
		pv->classification = realloc(pv->classification,
		    sizeof(char *) * pv->entries);
		pv->advisory = realloc(pv->advisory,
		    sizeof(char *) * pv->entries);
		if (pv->vulnerability == NULL ||
		    pv->classification == NULL || pv->advisory == NULL)
			errx(EXIT_FAILURE, "realloc failed");
	}

	return pv;
}

void
free_pkg_vulnerabilities(struct pkg_vulnerabilities *pv)
{
	size_t i;

	for (i = 0; i < pv->entries; ++i) {
		free(pv->vulnerability[i]);
		free(pv->classification[i]);
		free(pv->advisory[i]);
	}
	free(pv->vulnerability);
	free(pv->classification);
	free(pv->advisory);
	free(pv);
}
