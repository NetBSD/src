/*	$NetBSD: remote.c,v 1.1.1.1 2025/01/26 16:12:34 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <stdbool.h>
#include <string.h>

#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/name.h>
#include <dns/remote.h>
#include <dns/types.h>

isc_sockaddr_t *
dns_remote_addresses(dns_remote_t *remote) {
	REQUIRE(DNS_REMOTE_VALID(remote));
	return remote->addresses;
}

isc_sockaddr_t *
dns_remote_sources(dns_remote_t *remote) {
	REQUIRE(DNS_REMOTE_VALID(remote));
	return remote->sources;
}

unsigned int
dns_remote_count(dns_remote_t *remote) {
	REQUIRE(DNS_REMOTE_VALID(remote));
	return remote->addrcnt;
}

dns_name_t **
dns_remote_keynames(dns_remote_t *remote) {
	REQUIRE(DNS_REMOTE_VALID(remote));
	return remote->keynames;
}

dns_name_t **
dns_remote_tlsnames(dns_remote_t *remote) {
	REQUIRE(DNS_REMOTE_VALID(remote));
	return remote->tlsnames;
}

void
dns_remote_init(dns_remote_t *remote, unsigned int count,
		const isc_sockaddr_t *addrs, const isc_sockaddr_t *srcs,
		dns_name_t **keynames, dns_name_t **tlsnames, bool mark,
		isc_mem_t *mctx) {
	unsigned int i;

	REQUIRE(DNS_REMOTE_VALID(remote));
	REQUIRE(count == 0 || addrs != NULL);

	if (keynames != NULL || tlsnames != NULL) {
		REQUIRE(count != 0);
	}

	remote->mctx = mctx;

	if (addrs != NULL) {
		remote->addresses = isc_mem_cget(mctx, count,
						 sizeof(isc_sockaddr_t));
		memmove(remote->addresses, addrs,
			count * sizeof(isc_sockaddr_t));
	} else {
		remote->addresses = NULL;
	}

	if (srcs != NULL) {
		remote->sources = isc_mem_cget(mctx, count,
					       sizeof(isc_sockaddr_t));
		memmove(remote->sources, srcs, count * sizeof(isc_sockaddr_t));
	} else {
		remote->sources = NULL;
	}

	if (keynames != NULL) {
		remote->keynames = isc_mem_cget(mctx, count,
						sizeof(keynames[0]));
		for (i = 0; i < count; i++) {
			remote->keynames[i] = NULL;
		}
		for (i = 0; i < count; i++) {
			if (keynames[i] != NULL) {
				remote->keynames[i] =
					isc_mem_get(mctx, sizeof(dns_name_t));
				dns_name_init(remote->keynames[i], NULL);
				dns_name_dup(keynames[i], mctx,
					     remote->keynames[i]);
			}
		}
	} else {
		remote->keynames = NULL;
	}

	if (tlsnames != NULL) {
		remote->tlsnames = isc_mem_cget(mctx, count,
						sizeof(tlsnames[0]));
		for (i = 0; i < count; i++) {
			remote->tlsnames[i] = NULL;
		}
		for (i = 0; i < count; i++) {
			if (tlsnames[i] != NULL) {
				remote->tlsnames[i] =
					isc_mem_get(mctx, sizeof(dns_name_t));
				dns_name_init(remote->tlsnames[i], NULL);
				dns_name_dup(tlsnames[i], mctx,
					     remote->tlsnames[i]);
			}
		}
	} else {
		remote->tlsnames = NULL;
	}

	if (mark) {
		remote->ok = isc_mem_cget(mctx, count, sizeof(bool));
		for (i = 0; i < count; i++) {
			remote->ok[i] = false;
		}
	} else {
		remote->ok = NULL;
	}

	remote->addrcnt = count;
	remote->curraddr = 0;
}

static bool
same_addrs(isc_sockaddr_t const *oldlist, isc_sockaddr_t const *newlist,
	   uint32_t count) {
	unsigned int i;

	if (oldlist == NULL && newlist == NULL) {
		return true;
	}
	if (oldlist == NULL || newlist == NULL) {
		return false;
	}

	for (i = 0; i < count; i++) {
		if (!isc_sockaddr_equal(&oldlist[i], &newlist[i])) {
			return false;
		}
	}
	return true;
}

static bool
same_names(dns_name_t *const *oldlist, dns_name_t *const *newlist,
	   uint32_t count) {
	unsigned int i;

	if (oldlist == NULL && newlist == NULL) {
		return true;
	}
	if (oldlist == NULL || newlist == NULL) {
		return false;
	}

	for (i = 0; i < count; i++) {
		if (oldlist[i] == NULL && newlist[i] == NULL) {
			continue;
		}
		if (oldlist[i] == NULL || newlist[i] == NULL ||
		    !dns_name_equal(oldlist[i], newlist[i]))
		{
			return false;
		}
	}
	return true;
}

void
dns_remote_clear(dns_remote_t *remote) {
	unsigned int count;
	isc_mem_t *mctx;

	REQUIRE(DNS_REMOTE_VALID(remote));

	count = remote->addrcnt;
	mctx = remote->mctx;

	if (mctx == NULL) {
		return;
	}

	if (remote->ok != NULL) {
		isc_mem_cput(mctx, remote->ok, count, sizeof(bool));
		remote->ok = NULL;
	}

	if (remote->addresses != NULL) {
		isc_mem_cput(mctx, remote->addresses, count,
			     sizeof(isc_sockaddr_t));
		remote->addresses = NULL;
	}

	if (remote->sources != NULL) {
		isc_mem_cput(mctx, remote->sources, count,
			     sizeof(isc_sockaddr_t));
		remote->sources = NULL;
	}

	if (remote->keynames != NULL) {
		unsigned int i;
		for (i = 0; i < count; i++) {
			if (remote->keynames[i] != NULL) {
				dns_name_free(remote->keynames[i], mctx);
				isc_mem_put(mctx, remote->keynames[i],
					    sizeof(dns_name_t));
				remote->keynames[i] = NULL;
			}
		}
		isc_mem_cput(mctx, remote->keynames, count,
			     sizeof(dns_name_t *));
		remote->keynames = NULL;
	}

	if (remote->tlsnames != NULL) {
		unsigned int i;
		for (i = 0; i < count; i++) {
			if (remote->tlsnames[i] != NULL) {
				dns_name_free(remote->tlsnames[i], mctx);
				isc_mem_put(mctx, remote->tlsnames[i],
					    sizeof(dns_name_t));
				remote->tlsnames[i] = NULL;
			}
		}
		isc_mem_cput(mctx, remote->tlsnames, count,
			     sizeof(dns_name_t *));
		remote->tlsnames = NULL;
	}

	remote->curraddr = 0;
	remote->addrcnt = 0;
	remote->mctx = NULL;
}

bool
dns_remote_equal(dns_remote_t *a, dns_remote_t *b) {
	REQUIRE(DNS_REMOTE_VALID(a));
	REQUIRE(DNS_REMOTE_VALID(b));

	if (a->addrcnt != b->addrcnt) {
		return false;
	}

	if (!same_addrs(a->addresses, b->addresses, a->addrcnt)) {
		return false;
	}
	if (!same_names(a->keynames, b->keynames, a->addrcnt)) {
		return false;
	}
	if (!same_names(a->tlsnames, b->tlsnames, a->addrcnt)) {
		return false;
	}

	return true;
}

void
dns_remote_reset(dns_remote_t *remote, bool clear_ok) {
	REQUIRE(DNS_REMOTE_VALID(remote));

	remote->curraddr = 0;

	if (clear_ok && remote->ok != NULL) {
		for (unsigned int i = 0; i < remote->addrcnt; i++) {
			remote->ok[i] = false;
		}
	}
}

isc_sockaddr_t
dns_remote_curraddr(dns_remote_t *remote) {
	REQUIRE(DNS_REMOTE_VALID(remote));
	REQUIRE(remote->addresses != NULL);
	REQUIRE(remote->curraddr < remote->addrcnt);

	return remote->addresses[remote->curraddr];
}

isc_sockaddr_t
dns_remote_addr(dns_remote_t *remote, unsigned int i) {
	REQUIRE(DNS_REMOTE_VALID(remote));
	REQUIRE(remote->addresses != NULL);
	REQUIRE(i < remote->addrcnt);

	return remote->addresses[i];
}

isc_sockaddr_t
dns_remote_sourceaddr(dns_remote_t *remote) {
	REQUIRE(DNS_REMOTE_VALID(remote));
	REQUIRE(remote->sources != NULL);
	REQUIRE(remote->curraddr < remote->addrcnt);

	return remote->sources[remote->curraddr];
}

dns_name_t *
dns_remote_keyname(dns_remote_t *remote) {
	REQUIRE(DNS_REMOTE_VALID(remote));

	if (remote->keynames == NULL) {
		return NULL;
	}
	if (remote->curraddr >= remote->addrcnt) {
		return NULL;
	}

	return remote->keynames[remote->curraddr];
}

dns_name_t *
dns_remote_tlsname(dns_remote_t *remote) {
	REQUIRE(DNS_REMOTE_VALID(remote));

	if (remote->tlsnames == NULL) {
		return NULL;
	}
	if (remote->curraddr >= remote->addrcnt) {
		return NULL;
	}

	return remote->tlsnames[remote->curraddr];
}

void
dns_remote_next(dns_remote_t *remote, bool skip_good) {
	REQUIRE(DNS_REMOTE_VALID(remote));

skip_to_next:
	remote->curraddr++;

	if (remote->curraddr >= remote->addrcnt) {
		return;
	}

	if (skip_good && remote->ok != NULL && remote->ok[remote->curraddr]) {
		goto skip_to_next;
	}
}

bool
dns_remote_done(dns_remote_t *remote) {
	REQUIRE(DNS_REMOTE_VALID(remote));

	return remote->curraddr >= remote->addrcnt;
}

void
dns_remote_mark(dns_remote_t *remote, bool good) {
	REQUIRE(DNS_REMOTE_VALID(remote));
	REQUIRE(remote->curraddr < remote->addrcnt);

	remote->ok[remote->curraddr] = good;
}
