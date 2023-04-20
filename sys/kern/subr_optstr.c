/*	$NetBSD: subr_optstr.c,v 1.8 2023/04/20 09:04:45 skrll Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_optstr.c,v 1.8 2023/04/20 09:04:45 skrll Exp $");

#include <sys/param.h>
#include <sys/optstr.h>

/* --------------------------------------------------------------------- */

/*
 * Given an options string of the form 'a=b c=d ... y=z' and a key,
 * looks for the given key's value in the string and returns it in buf
 * with a maximum of bufsize bytes.  If the key is found, returns true;
 * otherwise FALSE.
 */


static bool
optstr_get_pointer(const char *optstr, const char *key, const char **result)
{
	bool found = false;

	/* Skip any initial spaces or tabs until we find a word. */
	while (*optstr == ' ' || *optstr == '\t')
		optstr++;

	/* Search for the given key within the option string. */
	while (!found && *optstr != '\0') {
		const char *keyp;

		/* Check if the next word matches the key. */
		keyp = key;
		while (*optstr == *keyp) {
			optstr++;
			keyp++;
		}

		if (*optstr == '=' && *keyp == '\0')
			found = true;
		else {
			/* Key not found; skip until next space. */
			while (*optstr != ' ' && *optstr != '\0')
				optstr++;

			/* And now skip until next word. */
			while (*optstr == ' ')
				optstr++;
		}
	}

	if (found) {
		optstr++; /* Skip '='. */
		*result = optstr;
	}

	return found;
}

bool
optstr_get(const char *optstr, const char *key, char *buf, size_t bufsize)
{
	const char *data;
	bool found = optstr_get_pointer(optstr, key, &data);

	/* If the key was found; copy its value to the target buffer. */
	if (found) {
		const char *lastbuf = buf + (bufsize - 1);

		while (buf != lastbuf && *data != ' ' && *data != '\0')
			*buf++ = *data++;
		*buf = '\0';
	}
	return found;
}


bool
optstr_get_string(const char *optstr, const char *key, const char **result)
{
	const char *data;
	const bool found = optstr_get_pointer(optstr, key, &data);

	/* If the key was found; copy its value to the target buffer. */
	if (found) {
		*result = data;
	}
	return found;
}

bool
optstr_get_number(const char *optstr, const char *key, unsigned long *result)
{
	const char *data;
	const bool found = optstr_get_pointer(optstr, key, &data);

	/* If the key was found; copy its value to the target buffer. */
	if (found) {
		char *ep;
		const unsigned long ulval = strtoul(data, &ep, 10);
		if (ep == data)
			return false;
		*result = ulval;
		return true;
	}
	return false;
}

bool
optstr_get_number_hex(const char *optstr, const char *key,
    unsigned long *result)
{
	const char *data;
	const bool found = optstr_get_pointer(optstr, key, &data);

	/* If the key was found; copy its value to the target buffer. */
	if (found) {
		char *ep;
		const unsigned long ulval = strtoul(data, &ep, 16);
		if (ep == data)
			return false;
		*result = ulval;
		return true;
	}
	return false;
}

bool
optstr_get_number_binary(const char *optstr, const char *key,
    unsigned long *result)
{
	const char *data;
	const bool found = optstr_get_pointer(optstr, key, &data);

	/* If the key was found; copy its value to the target buffer. */
	if (found) {
		char *ep;
		const unsigned long ulval = strtoul(data, &ep, 2);
		if (ep == data)
			return false;
		*result = ulval;
		return true;
	}
	return false;
}


#if NETHER > 0
bool
optstr_get_macaddr(const char *optstr, const char *key,
    uint8_t result[ETHER_ADDR_LEN])
{
	const char *data;
	const bool found = optstr_get_pointer(optstr, key, &data);

	/* If the key was found; copy its value to the target buffer. */
	if (found) {
		uint8_t temp[ETHER_ADDR_LEN];
		int error = ether_aton_r(temp, sizeof(temp), data);
		if (error)
			return false;
		memcpy(result, temp, sizeof(temp));
	}
	return found;
}
#endif
