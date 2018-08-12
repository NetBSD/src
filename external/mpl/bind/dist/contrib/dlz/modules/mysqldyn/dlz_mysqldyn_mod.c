/*	$NetBSD: dlz_mysqldyn_mod.c,v 1.2 2018/08/12 13:02:32 christos Exp $	*/

/*
 * Copyright (C) 2014 Maui Systems Ltd, Scotland, contact@maui-systems.co.uk.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND MAUI SYSTEMS LTD DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MAUI SYSTEMS LTD  BE
 * LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Copyright (C) 2011,2014  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * BIND 9 DLZ MySQL module with support for dynamic DNS (DDNS)
 *
 * Adapted from code contributed by Marty Lee, Maui Systems Ltd.
 *
 * See README for database schema and usage details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <pthread.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <mysql/mysql.h>
#include <mysql/errmsg.h>

#include <dlz_minimal.h>
#include <dlz_list.h>
#include <dlz_pthread.h>

/*
 * The SQL queries that will be used for lookups and updates are defined
 * here.  They will be processed into queries by the build_query()
 * function.
 *
 * NOTE: Despite appearances, these do NOT use printf-style formatting.
 * "%s", with no modifiers, is the only supported directive.
 */

/*
 * Get the NS RRset for a zone
 * Arguments: zone-name
 */
#define Q_GETNS \
	"SELECT d.data FROM ZoneData d, Zones z " \
	"WHERE UPPER(d.type) = 'NS' AND LOWER(z.domain) = LOWER('%s') " \
	"AND z.id = d.zone_id"

/*
 * Get a list of zones (ignoring writable or not)
 * Arguments: (none)
 */
#define Q_GETZONES "SELECT LOWER(domain), serial FROM Zones"

/*
 * Find a specific zone
 * Arguments: zone-name
 */
#define Q_FINDZONE \
	"SELECT id FROM Zones WHERE LOWER(domain) = LOWER('%s')"

/*
 * Get SOA data from zone apex
 * Arguments: zone-name
 */
#define Q_GETSOA \
	"SELECT host, admin, serial, refresh, retry, expire, minimum, ttl " \
	"FROM Zones WHERE LOWER(domain) = LOWER('%s')"

/*
 * Get other data from zone apex
 * Arguments: zone-name, zone-name (repeated)
 */
#define Q_GETAPEX \
	"SELECT d.type, d.data, d.ttl FROM ZoneData d, Zones z " \
	"WHERE LOWER(z.domain) = LOWER('%s') AND z.id = d.zone_id " \
	"AND LOWER(d.name) IN (LOWER('%s'), '', '@') "\
	"ORDER BY UPPER(d.type) ASC"

/*
 * Get data from non-apex nodes
 * Arguments: zone-name, node-name (relative to zone name)
 */
#define Q_GETNODE \
	"SELECT d.type, d.data, d.ttl FROM ZoneData d, Zones z " \
	"WHERE LOWER(z.domain) = LOWER('%s') AND z.id = d.zone_id " \
	"AND LOWER(d.name) = LOWER('%s') " \
	"ORDER BY UPPER(d.type) ASC"

/*
 * Get all data from a zone, for AXFR
 * Arguments: zone-name
 */
#define Q_GETALL \
	"SELECT d.name, d.type, d.data, d.ttl FROM ZoneData d, Zones z " \
	"WHERE LOWER(z.domain) = LOWER('%s') AND z.id = d.zone_id"

/*
 * Get SOA serial number for a zone.
 * Arguments: zone-name
 */
#define Q_GETSERIAL \
	"SELECT serial FROM Zones WHERE domain = '%s'"

/*
 * Determine whether a zone is writeable, and if so, retrieve zone_id
 * Arguments: zone-name
 */
#define Q_WRITEABLE \
	"SELECT id FROM Zones WHERE " \
	"LOWER(domain) = LOWER('%s') AND writeable = 1"

/*
 * Insert data into zone (other than SOA)
 * Arguments: zone-id (from Q_WRITEABLE), node-name (relative to zone-name),
 * 	      rrtype, rdata text, TTL (in text format)
 */
#define I_DATA \
	"INSERT INTO ZoneData (zone_id, name, type, data, ttl) " \
	"VALUES (%s, LOWER('%s'), UPPER('%s'), '%s', %s)"

/*
 * Update SOA serial number for a zone
 * Arguments: new serial number (in text format), zone-id (from Q_WRITEABLE)
 */
#define U_SERIAL \
	"UPDATE Zones SET serial = %s WHERE id = %s"

/*
 * Delete a specific record (non-SOA) from a zone
 *
 * Arguments: node-name (relative to zone-name), zone-id (from Q_WRITEABLE),
 * 	      rrtype, rdata text, TTL (in text format).
 */
#define D_RECORD \
	"DELETE FROM ZoneData WHERE zone_id = %s AND " \
	"LOWER(name) = LOWER('%s') AND UPPER(type) = UPPER('%s') AND " \
	"data = '%s' AND ttl = %s"

/*
 * Delete an entire rrset from a zone
 * Arguments: node-name (relative to zone-name), zone-id (from Q_WRITEABLE),
 * 	      rrtype.
 */
#define D_RRSET \
	"DELETE FROM ZoneData WHERE zone_id = %s AND " \
	"LOWER(name) = LOWER('%s') AND UPPER(type) = UPPER('%s')"

#ifdef WIN32
#define STRTOK_R(a, b, c)       strtok_s(a, b, c)
#elif defined(_REENTRANT)
#define STRTOK_R(a, b, c)       strtok_r(a, b, c)
#else
#define STRTOK_R(a, b, c)       strtok(a, b)
#endif

/*
 * Number of concurrent database connections we support
 * - equivalent to maxmium number of concurrent transactions
 *   that can be 'in-flight' + 1
 */
#define MAX_DBI 16

typedef struct mysql_record {
	char zone[255];
	char name[100];
	char type[10];
	char data[200];
	char ttl[10];
} mysql_record_t;

typedef struct mysql_instance {
	int id;
	MYSQL *sock;
	int connected;
	dlz_mutex_t mutex;
} mysql_instance_t;

typedef struct mysql_transaction mysql_transaction_t;
struct mysql_transaction {
	char *zone;
	char *zone_id;
	mysql_instance_t *dbi;
	mysql_transaction_t *next;
};

typedef struct mysql_data {
	int debug;

	/*
	 * Database connection details
	 */
	char *db_name;
	char *db_host;
	char *db_user;
	char *db_pass;

	/*
	 * Database structures
	 */
	mysql_instance_t db[MAX_DBI];

	/*
	 * Transactions
	 */
	mysql_transaction_t *transactions;

	/*
	 * Mutex for transactions
	 */
	dlz_mutex_t tx_mutex;

	/* Helper functions from the dlz_dlopen driver */
	log_t *log;
	dns_sdlz_putrr_t *putrr;
	dns_sdlz_putnamedrr_t *putnamedrr;
	dns_dlz_writeablezone_t *writeable_zone;
} mysql_data_t;

typedef struct mysql_arg mysql_arg_t;
typedef DLZ_LIST(mysql_arg_t) mysql_arglist_t;
struct mysql_arg {
	char *arg;
	DLZ_LINK(mysql_arg_t) link;
};

static const char *modname = "dlz_mysqldyn";

/*
 * Local functions
 */
static isc_boolean_t
db_connect(mysql_data_t *state, mysql_instance_t *dbi) {
	MYSQL *conn;
	/*
	 * Make sure this thread has been through 'init'
	 */
	mysql_thread_init();

	if (dbi->connected)
		return (ISC_TRUE);

	if (state->log != NULL)
		state->log(ISC_LOG_INFO, "%s: init connection %d ",
			   modname, dbi->id);

	conn = mysql_real_connect(dbi->sock, state->db_host,
				  state->db_user, state->db_pass,
				  state->db_name, 0, NULL,
				  CLIENT_REMEMBER_OPTIONS);
	if (conn == NULL) {
		if (state->log != NULL)
			state->log(ISC_LOG_ERROR,
				   "%s: database connection failed: %s",
				   modname, mysql_error(dbi->sock));

		dlz_mutex_unlock(&dbi->mutex);
		return (ISC_FALSE);
	}

	dbi->connected = 1;
	return (ISC_TRUE);
}

static mysql_instance_t *
get_dbi(mysql_data_t *state) {
	int i;

	/*
	 * Find an available dbi
	 */
	for (i = 0; i < MAX_DBI; i++) {
		if (dlz_mutex_trylock(&state->db[i].mutex) == 0)
			break;
	}

	if (i == MAX_DBI) {
		if (state->debug && state->log != NULL)
			state->log(ISC_LOG_ERROR,
				   "%s: No available connections", modname);
		return (NULL);
	}
	return (&state->db[i]);
}

/*
 * Allocate memory and store an escaped, sanitized version
 * of string 'original'
 */
static char *
sanitize(mysql_instance_t *dbi, const char *original) {
	char *s;

	if (original == NULL)
		return (NULL);

	s = (char *) malloc((strlen(original) * 2) + 1);
	if (s != NULL) {
		memset(s, 0, (strlen(original) * 2) + 1);

		mysql_real_escape_string(dbi->sock, s, original,
					 strlen(original));
	}

	return (s);
}

/*
 * Append the string pointed to by 's' to the argument list 'arglist',
 * and add the string length to the running total pointed to by 'len'.
 */
static isc_result_t
additem(mysql_arglist_t *arglist, char **s, size_t *len) {
	mysql_arg_t *item;

	item = malloc(sizeof(*item));
	if (item == NULL)
		return (ISC_R_NOMEMORY);

	DLZ_LINK_INIT(item, link);
	item->arg = *s;
	*len += strlen(*s);
	DLZ_LIST_APPEND(*arglist, item, link);
	*s = NULL;

	return (ISC_R_SUCCESS);
}

/*
 * Construct a query string using a variable number of arguments, and
 * save it into newly allocated memory.
 *
 * NOTE: 'command' resembles a printf-style format string, but ONLY
 * supports the "%s" directive with no modifiers of any kind.
 *
 * If 'dbi' is NULL, we attempt to get a temporary database connection;
 * otherwise we use the existing one.
 */
static char *
build_query(mysql_data_t *state, mysql_instance_t *dbi,
	    const char *command, ...)
{
	isc_result_t result;
	isc_boolean_t localdbi = ISC_FALSE;
	mysql_arglist_t arglist;
	mysql_arg_t *item;
	char *p, *q, *tmp = NULL, *querystr = NULL;
	char *query = NULL;
	size_t len = 0;
	va_list ap1;

	/* Get a DB instance if needed */
	if (dbi == NULL) {
		dbi = get_dbi(state);
		if (dbi == NULL)
			return (NULL);
		localdbi = ISC_TRUE;
	}

	/* Make sure this instance is connected */
	if (!db_connect(state, dbi))
		goto fail;

	va_start(ap1, command);
	DLZ_LIST_INIT(arglist);
	q = querystr = strdup(command);
	if (querystr == NULL)
		goto fail;

	for (;;) {
		if (*q == '\0')
			break;

		p = strstr(q, "%s");
		if (p != NULL) {
			*p = '\0';
			tmp = strdup(q);
			if (tmp == NULL)
				goto fail;

			result = additem(&arglist, &tmp, &len);
			if (result != ISC_R_SUCCESS)
				goto fail;

			tmp = sanitize(dbi, va_arg(ap1, const char *));
			if (tmp == NULL)
				goto fail;

			result = additem(&arglist, &tmp, &len);
			if (result != ISC_R_SUCCESS)
				goto fail;

			q = p + 2;
		} else {
			tmp = strdup(q);
			if (tmp == NULL)
				goto fail;

			result = additem(&arglist, &tmp, &len);
			if (result != ISC_R_SUCCESS)
				goto fail;

			break;
		}
	}

	if (len == 0)
		goto fail;

	query = malloc(len + 1);
	if (query == NULL)
		goto fail;

	*query = '\0';
	for (item = DLZ_LIST_HEAD(arglist);
	     item != NULL;
	     item = DLZ_LIST_NEXT(item, link))
		if (item->arg != NULL)
			strcat(query, item->arg);

 fail:
	va_end(ap1);

	for (item = DLZ_LIST_HEAD(arglist);
	     item != NULL;
	     item = DLZ_LIST_NEXT(item, link))
	{
		if (item->arg != NULL)
			free(item->arg);
		free(item);
	}

	if (tmp != NULL)
		free(tmp);
	if (querystr != NULL)
		free (querystr);

	if (dbi != NULL && localdbi)
		dlz_mutex_unlock(&dbi->mutex);

	return (query);
}

/* Does this name end in a dot? */
static isc_boolean_t
isrelative(const char *s) {
	if (s == NULL || s[strlen(s) - 1] == '.')
		return (ISC_FALSE);
	return (ISC_TRUE);
}

/* Return a dot if 's' doesn't already end with one */
static inline const char *
dot(const char *s) {
	return (isrelative(s) ? "." : "");
}

/*
 * Generate a full hostname from a (presumably relative) name 'name'
 * and a zone name 'zone'; store the result in 'dest' (which must have
 * enough space).
 */
static void
fqhn(const char *name, const char *zone, char *dest) {
	if (dest == NULL)
		return;

	if (strlen(name) == 0 || strcmp(name, "@") == 0)
		sprintf(dest, "%s%s", zone, dot(zone));
	else {
		if (isrelative(name))
			sprintf(dest, "%s.%s%s", name, zone, dot(zone));
		else
			strcpy(dest, name);
	}
}

/*
 * Names are stored in relative form in ZoneData; this function
 * removes labels matching 'zone' from the end of 'name'.
 */
static char *
relname(const char *name, const char *zone) {
	size_t nlen, zlen;
	const char *p;
	char *new;

	new = (char *) malloc(strlen(name) + 1);
	if (new == NULL)
		return (NULL);

	nlen = strlen(name);
	zlen = strlen(zone);

	if (nlen < zlen) {
		strcpy(new, name);
		return (new);
	} else if (nlen == zlen || strcasecmp(name, zone) == 0) {
		strcpy(new, "@");
		return (new);
	}

	p = name + nlen - zlen;
	if (strcasecmp(p, zone) != 0 &&
	    (zone[zlen - 1] != '.' ||
	     strncasecmp(p, zone, zlen - 1) != 0))
	{
		strcpy(new, name);
		return (new);
	}

	strncpy(new, name, nlen - zlen);
	new[nlen - zlen - 1] = '\0';
	return (new);
}

static isc_result_t
validate_txn(mysql_data_t *state, mysql_transaction_t *txn) {
	isc_result_t result = ISC_R_FAILURE;
	mysql_transaction_t *txp;

	dlz_mutex_lock(&state->tx_mutex);
	for (txp = state->transactions; txp != NULL; txp = txp->next) {
		if (txn == txp) {
			result = ISC_R_SUCCESS;
			break;
		}
	}
	dlz_mutex_unlock(&state->tx_mutex);

	if (result != ISC_R_SUCCESS && state->log != NULL)
		state->log(ISC_LOG_ERROR, "%s: invalid txn %x", modname, txn);

	return (result);
}

static isc_result_t
db_execute(mysql_data_t *state, mysql_instance_t *dbi, const char *query) {
	int ret;

	/* Make sure this instance is connected.  */
	if (!db_connect(state, dbi))
		return (ISC_R_FAILURE);

	ret = mysql_real_query(dbi->sock, query, strlen(query));
	if (ret != 0) {
		if (state->debug && state->log != NULL)
			state->log(ISC_LOG_ERROR,
				   "%s: query '%s' failed: %s",
				   modname, query, mysql_error(dbi->sock));
		return (ISC_R_FAILURE);
	}

	if (state->debug && state->log != NULL)
		state->log(ISC_LOG_INFO, "%s: execute(%d) %s",
			   modname, dbi->id, query);

	return (ISC_R_SUCCESS);
}

static MYSQL_RES *
db_query(mysql_data_t *state, mysql_instance_t *dbi, const char *query) {
	isc_result_t result;
	isc_boolean_t localdbi = ISC_FALSE;
	MYSQL_RES *res = NULL;

	if (query == NULL)
		return (NULL);

	/* Get a DB instance if needed */
	if (dbi == NULL) {
		dbi = get_dbi(state);
		if (dbi == NULL)
			return (NULL);
		localdbi = ISC_TRUE;
	}

	/* Make sure this instance is connected */
	if (!db_connect(state, dbi))
		goto fail;

	result = db_execute(state, dbi, query);
	if (result != ISC_R_SUCCESS)
		goto fail;

	res = mysql_store_result(dbi->sock);
	if (res == NULL) {
		if (state->log != NULL)
			state->log(ISC_LOG_ERROR,
				   "%s: unable to store result: %s",
				   modname, mysql_error(dbi->sock));
		goto fail;
	}

	if (state->debug && state->log != NULL)
		state->log(ISC_LOG_INFO,
			   "%s: query(%d) returned %d rows",
			   modname, dbi->id, mysql_num_rows(res));

 fail:
	if (dbi != NULL && localdbi)
		dlz_mutex_unlock(&dbi->mutex);
	return (res);
}

/*
 * Generate a DNS NOTIFY packet:
 * 12 bytes header
 * Question (1)
 * 	strlen(zone) +2
 *  2 bytes qtype
 *  2 bytes qclass
 *
 * -> 18 bytes + strlen(zone)
 *
 * N.B. Need to be mindful of byte ordering; using htons to map 16bit
 * values to the 'on the wire' packet values.
 */
static unsigned char *
make_notify(const char *zone, int *packetlen) {
	int i, j;
	unsigned char *packet = (unsigned char *) malloc(strlen(zone) + 18);

	if (packet == NULL)
		return (NULL);

	*packetlen = strlen(zone) + 18;
	memset(packet, 0, *packetlen);

	/* Random query ID */
	i = rand();
	packet[0] = htons(i) & 0xff;
	packet[1] = htons(i) >> 8;

	/* Flags (OpCode '4' in bits 14-11), Auth Answer set in bit 10 */
	i = 0x2400;
	packet[2] = htons(i) & 0xff;
	packet[3] = htons(i) >> 8;

	/* QD Count */
	i = 0x1;
	packet[4] = htons(i) & 0xff;
	packet[5] = htons(i) >> 8;

	/* Question */
	packet[12] = '.';
	memcpy(&packet[13], zone, strlen(zone));
	packet[13 + strlen(zone)] = 0;

	/* Make the question into labels */
	j = 12;
	while (packet[j]) {
		for (i = j + 1; packet[i] != '\0' && packet[i] != '.'; i++);
		packet[j] = i - j - 1;
		j = i;
	}

	/* Question type */
	i = 6;
	packet[j + 1] = htons(i) & 0xff;
	packet[j + 2] = htons(i) >> 8;

	/* Queston class */
	i = 1;
	packet[j + 3] = htons(i) & 0xff;
	packet[j + 4] = htons(i) >> 8;

	return (packet);
}

static void
send_notify(struct sockaddr_in *addr, const unsigned char *p, const int plen) {
	int s;

	addr->sin_family = AF_INET;
	addr->sin_port = htons(53);

	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		return;

	sendto(s, p, plen, 0, (struct sockaddr *)addr, sizeof(*addr));
	close(s);
	return;
}

/*
 * Generate and send a DNS NOTIFY packet
 */
static void
notify(mysql_data_t *state, const char *zone, int sn) {
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *query;
	unsigned char *packet;
	int packetlen;
	struct ifaddrs *ifap, *ifa;
	char zaddr[INET_ADDRSTRLEN];
	void *addrp = NULL;

	/* Get the name servers from the NS rrset */
	query = build_query(state, NULL, Q_GETNS, zone);
	res = db_query(state, NULL, query);
	free (query);
	if (res == NULL)
		return;

	/* Create a DNS NOTIFY packet */
	packet = make_notify(zone, &packetlen);
	if (packet == NULL) {
		mysql_free_result(res);
		return;
	}

	/* Get a list of our own addresses */
	if (getifaddrs(&ifap) < 0)
		ifap = NULL;

	/* Tell each nameserver of the update */
	while ((row = mysql_fetch_row(res)) != NULL) {
		isc_boolean_t local = ISC_FALSE;
		struct hostent *h;
		struct sockaddr_in addr, *sin;

		/*
		 * Put nameserver rdata through gethostbyname as it
		 * might be an IP address or a hostname. (XXX: switch
		 * this to inet_pton/getaddrinfo.)
		 */
		h = gethostbyname(row[0]);
		if (h == NULL)
			continue;

		memcpy(&addr.sin_addr, h->h_addr, h->h_length);
		addrp = &addr.sin_addr;

		/* Get the address for the nameserver into a string */
		inet_ntop(AF_INET, addrp, zaddr, INET_ADDRSTRLEN);
		for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
			char ifaddr[INET_ADDRSTRLEN];

			if (ifa->ifa_addr->sa_family != AF_INET)
				continue;

			/* Get local address into a string */
			sin = (struct sockaddr_in *) ifa->ifa_addr;
			addrp = &sin->sin_addr;
			inet_ntop(AF_INET, addrp, ifaddr, INET_ADDRSTRLEN);

			/* See if nameserver address matches this one */
			if (strcmp(ifaddr, zaddr) == 0)
				local = ISC_TRUE;
		}

		if (!local) {
			if (state->log != NULL)
				state->log(ISC_LOG_INFO,
					   "%s: notify %s zone %s serial %d",
					   modname, row[0], zone, sn);
			send_notify(&addr, packet, packetlen);
		}
	}

	mysql_free_result(res);
	free(packet);
	if (ifap != NULL)
		freeifaddrs(ifap);
}

/*
 * Constructs a mysql_record_t structure from 'rdatastr', to be
 * used in the dlz_{add,sub,del}rdataset functions below.
 */
static mysql_record_t *
makerecord(mysql_data_t *state, const char *name, const char *rdatastr) {
	mysql_record_t *new_record;
	char *real_name, *dclass, *type, *data, *ttlstr, *buf;
	dns_ttl_t ttlvalue;

	new_record = (mysql_record_t *)
		malloc(sizeof(mysql_record_t));

	if (new_record == NULL) {
		if (state->log != NULL)
			state->log(ISC_LOG_ERROR,
				   "%s: makerecord - unable to malloc",
				   modname);
		return (NULL);
	}

	buf = strdup(rdatastr);
	if (buf == NULL) {
		if (state->log != NULL)
			state->log(ISC_LOG_ERROR,
				   "%s: makerecord - unable to malloc",
				   modname);
		free(new_record);
		return (NULL);
	}

	/*
	 * The format is:
	 * FULLNAME\tTTL\tDCLASS\tTYPE\tDATA
	 *
	 * The DATA field is space separated, and is in the data format
	 * for the type used by dig
	 */
	real_name = STRTOK_R(buf, "\t", &saveptr);
	if (real_name == NULL)
		goto error;

	ttlstr = STRTOK_R(NULL, "\t", &saveptr);
	if (ttlstr == NULL || sscanf(ttlstr, "%d", &ttlvalue) != 1)
		goto error;

	dclass = STRTOK_R(NULL, "\t", &saveptr);
	if (dclass == NULL)
		goto error;

	type = STRTOK_R(NULL, "\t", &saveptr);
	if (type == NULL)
		goto error;

	data = STRTOK_R(NULL, "\t", &saveptr);
	if (data == NULL)
		goto error;

	strcpy(new_record->name, name);
	strcpy(new_record->type, type);
	strcpy(new_record->data, data);
	sprintf(new_record->ttl, "%d", ttlvalue);

	free(buf);
	return (new_record);

 error:
	free(buf);
	free(new_record);
	return (NULL);
}

/*
 * Remember a helper function from the bind9 dlz_dlopen driver
 */
static void
b9_add_helper(mysql_data_t *state, const char *helper_name, void *ptr) {
	if (strcmp(helper_name, "log") == 0)
		state->log = (log_t *)ptr;
	if (strcmp(helper_name, "putrr") == 0)
		state->putrr = (dns_sdlz_putrr_t *)ptr;
	if (strcmp(helper_name, "putnamedrr") == 0)
		state->putnamedrr = (dns_sdlz_putnamedrr_t *)ptr;
	if (strcmp(helper_name, "writeable_zone") == 0)
		state->writeable_zone = (dns_dlz_writeablezone_t *)ptr;
}

/*
 * DLZ API functions
 */

/*
 * Return the version of the API
 */
int
dlz_version(unsigned int *flags) {
	UNUSED(flags);
	*flags |= DNS_SDLZFLAG_THREADSAFE;
	return (DLZ_DLOPEN_VERSION);
}

/*
 * Called to initialize the driver
 */
isc_result_t
dlz_create(const char *dlzname, unsigned int argc, char *argv[],
	   void **dbdata, ...)
{
	mysql_data_t *state;
	const char *helper_name;
	va_list ap;
	int n;

	UNUSED(dlzname);

	state = calloc(1, sizeof(mysql_data_t));
	if (state == NULL)
		return (ISC_R_NOMEMORY);

	dlz_mutex_init(&state->tx_mutex, NULL);
	state->transactions = NULL;

	/* Fill in the helper functions */
	va_start(ap, dbdata);
	while ((helper_name = va_arg(ap, const char *)) != NULL)
		b9_add_helper(state, helper_name, va_arg(ap, void *));
	va_end(ap);

	if (state->log != NULL)
		state->log(ISC_LOG_INFO, "loading %s module", modname);

	if ((argc < 2) || (argc > 6)) {
		if (state->log != NULL)
			state->log(ISC_LOG_ERROR,
				   "%s: missing args <dbname> "
				   "[<dbhost> [<user> <pass>]]", modname);
		dlz_destroy(state);
		return (ISC_R_FAILURE);
	}

	state->db_name = strdup(argv[1]);
	if (argc > 2) {
		state->db_host = strdup(argv[2]);
		if (argc > 4) {
			state->db_user = strdup(argv[3]);
			state->db_pass = strdup(argv[4]);
		} else {
			state->db_user = strdup("bind");
			state->db_pass = strdup("");
		}
	} else {
		state->db_host = strdup("localhost");
		state->db_user = strdup("bind");
		state->db_pass = strdup("");
	}

	if (state->log != NULL)
		state->log(ISC_LOG_INFO,
			   "%s: DB=%s, Host=%s, User=%s",
			   modname, state->db_name,
			   state->db_host, state->db_user);

	/*
	 * Assign the 'state' to dbdata so we get it in our callbacks
	 */


	dlz_mutex_lock(&state->tx_mutex);

	/*
	 * Populate DB instances
	 */
	if (mysql_thread_safe()) {
		for (n = 0; n < MAX_DBI; n++) {
			my_bool opt = 1;
			dlz_mutex_init(&state->db[n].mutex, NULL);
			dlz_mutex_lock(&state->db[n].mutex);
			state->db[n].id = n;
			state->db[n].connected = 0;
			state->db[n].sock = mysql_init(NULL);
			mysql_options(state->db[n].sock,
				      MYSQL_READ_DEFAULT_GROUP,
				      modname);
			mysql_options(state->db[n].sock,
				      MYSQL_OPT_RECONNECT, &opt);
			dlz_mutex_unlock(&state->db[n].mutex);
		}

		*dbdata = state;
		dlz_mutex_unlock(&state->tx_mutex);
		return (ISC_R_SUCCESS);
	}

	free(state->db_name);
	free(state->db_host);
	free(state->db_user);
	free(state->db_pass);
	dlz_mutex_destroy(&state->tx_mutex);
	free(state);
	return (ISC_R_FAILURE);
}

/*
 * Shut down the backend
 */
void
dlz_destroy(void *dbdata) {
	mysql_data_t *state = (mysql_data_t *)dbdata;
	int i;

	if (state->debug && state->log != NULL)
		state->log(ISC_LOG_INFO, "%s: shutting down", modname);

	for (i = 0; i < MAX_DBI; i++) {
		if (state->db[i].sock) {
			mysql_close(state->db[i].sock);
			state->db[i].sock=NULL;
		}
	}
	free(state->db_name);
	free(state->db_host);
	free(state->db_user);
	free(state->db_pass);
	dlz_mutex_destroy(&state->tx_mutex);
	free(state);
}

/*
 * See if we handle a given zone
 */
isc_result_t
dlz_findzonedb(void *dbdata, const char *name,
	       dns_clientinfomethods_t *methods,
	       dns_clientinfo_t *clientinfo)
{
	isc_result_t result = ISC_R_SUCCESS;
	mysql_data_t *state = (mysql_data_t *)dbdata;
	MYSQL_RES *res;
	char *query;

	/* Query the Zones table to see if this zone is present */
	query = build_query(state, NULL, Q_FINDZONE, name);

	if (query == NULL)
		return (ISC_R_NOMEMORY);

	res = db_query(state, NULL, query);
	if (res == NULL)
		return (ISC_R_FAILURE);

	if (mysql_num_rows(res) == 0)
		result = ISC_R_NOTFOUND;

	mysql_free_result(res);
	return (result);
}

/*
 * Perform a database lookup
 */
isc_result_t
dlz_lookup(const char *zone, const char *name, void *dbdata,
	   dns_sdlzlookup_t *lookup, dns_clientinfomethods_t *methods,
	   dns_clientinfo_t *clientinfo)
{
	isc_result_t result;
	mysql_data_t *state = (mysql_data_t *)dbdata;
	isc_boolean_t found = ISC_FALSE;
	char *real_name;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *query;
	mysql_transaction_t *txn = NULL;
	mysql_instance_t *dbi = NULL;

	if (state->putrr == NULL) {
		if (state->log != NULL)
			state->log(ISC_LOG_ERROR,
				   "%s: dlz_lookup - no putrr", modname);
		return (ISC_R_NOTIMPLEMENTED);
	}

	/* Are we okay to try to find the txn version?  */
	if (clientinfo != NULL &&
	    clientinfo->version >= DNS_CLIENTINFO_VERSION) {
		txn = (mysql_transaction_t *) clientinfo->dbversion;
		if (txn != NULL && validate_txn(state, txn) == ISC_R_SUCCESS)
			dbi = txn->dbi;
		if (dbi != NULL) {
			state->log(ISC_LOG_DEBUG(1),
				   "%s: lookup in live transaction %p, DBI %p",
				   modname, txn, dbi);
		}
	}

	if (strcmp(name, "@") == 0) {
		real_name = (char *) malloc(strlen(zone) + 1);
		if (real_name == NULL)
			return (ISC_R_NOMEMORY);
		strcpy(real_name, zone);
	} else {
		real_name = (char *) malloc(strlen(name) + 1);
		if (real_name == NULL)
			return (ISC_R_NOMEMORY);
		strcpy(real_name, name);
	}

	if (strcmp(real_name, zone) == 0) {
		/*
		 * Get the Zones table data for use in the SOA:
		 * zone admin serial refresh retry expire min
		 */
		query = build_query(state, dbi, Q_GETSOA, zone);
		if (query == NULL) {
			free(real_name);
			return (ISC_R_NOMEMORY);
		}

		res = db_query(state, dbi, query);
		free (query);

		if (res == NULL) {
			free(real_name);
			return (ISC_R_NOTFOUND);
		}

		while ((row = mysql_fetch_row(res)) != NULL) {
			char host[1024], admin[1024], data[1024];
			int ttl;

			sscanf(row[7], "%d", &ttl);
			fqhn(row[0], zone, host);
			fqhn(row[1], zone, admin);

			/* zone admin serial refresh retry expire min */
			sprintf(data, "%s%s %s%s %s %s %s %s %s",
				host, dot(host), admin, dot(admin),
				row[2], row[3], row[4], row[5], row[6]);

			result = state->putrr(lookup, "soa", ttl, data);
			if (result != ISC_R_SUCCESS) {
				free(real_name);
				mysql_free_result(res);
				return (result);
			}
		}

		mysql_free_result(res);

		/*
		 *  Now we'll get the rest of the apex data
		 */
		query = build_query(state, dbi, Q_GETAPEX, zone, real_name);
	} else
		query = build_query(state, dbi, Q_GETNODE, zone, real_name);

	res = db_query(state, dbi, query);
	free(query);

	if (res == NULL) {
		free(real_name);
		return (ISC_R_NOTFOUND);
	}

	while ((row = mysql_fetch_row(res)) != NULL) {
		int ttl;
		sscanf(row[2], "%d", &ttl);
		result = state->putrr(lookup, row[0], ttl, row[1]);
		if (result != ISC_R_SUCCESS) {
			free(real_name);
			mysql_free_result(res);
			return (result);
		}

		found = ISC_TRUE;
	}

	if (state->debug && state->log != NULL)
		state->log(ISC_LOG_INFO,
			   "%s: dlz_lookup %s/%s/%s - (%d rows)",
			   modname, name, real_name, zone,
			   mysql_num_rows(res));

	mysql_free_result(res);
	free(real_name);

	if (!found)
		return (ISC_R_NOTFOUND);

	return (ISC_R_SUCCESS);
}

/*
 * See if a zone transfer is allowed
 */
isc_result_t
dlz_allowzonexfr(void *dbdata, const char *name, const char *client) {
	mysql_data_t *state = (mysql_data_t *)dbdata;

	if (state->debug && state->log != NULL)
		state->log(ISC_LOG_INFO,
			   "dlz_allowzonexfr: %s %s", name, client);

	/* Just say yes for all our zones */
	return (dlz_findzonedb(dbdata, name, NULL, NULL));
}

/*
 * Perform a zone transfer
 */
isc_result_t
dlz_allnodes(const char *zone, void *dbdata, dns_sdlzallnodes_t *allnodes) {
	isc_result_t result = ISC_R_SUCCESS;
	mysql_data_t *state = (mysql_data_t *)dbdata;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *query;

	UNUSED(zone);

	if (state->debug && (state->log != NULL))
		state->log(ISC_LOG_INFO, "dlz_allnodes: %s", zone);

	if (state->putnamedrr == NULL)
		return (ISC_R_NOTIMPLEMENTED);

	/*
	 * Get all the ZoneData for this zone
	 */
	query = build_query(state, NULL, Q_GETALL, zone);
	if (query == NULL)
		return (ISC_R_NOMEMORY);

	res = db_query(state, NULL, query);
	free(query);
	if (res == NULL)
		return (ISC_R_NOTFOUND);

	while ((row = mysql_fetch_row(res)) != NULL) {
		char hostname[1024];
		int ttl;

		sscanf(row[3], "%d", &ttl);
		fqhn(row[0], zone, hostname);
		result = state->putnamedrr(allnodes, hostname,
					   row[1], ttl, row[2]);
		if (result != ISC_R_SUCCESS)
			break;
	}

	mysql_free_result(res);
	return (result);
}

/*
 * Start a transaction
 */
isc_result_t
dlz_newversion(const char *zone, void *dbdata, void **versionp) {
	isc_result_t result = ISC_R_FAILURE;
	mysql_data_t *state = (mysql_data_t *) dbdata;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *query;
	char zone_id[16];
	mysql_transaction_t *txn = NULL, *newtx = NULL;

	/*
	 * Check Zone is writable
	 */
	query = build_query(state, NULL, Q_WRITEABLE, zone);
	if (query == NULL)
		return (ISC_R_NOMEMORY);

	res = db_query(state, NULL, query);
	free(query);
	if (res == NULL)
		return (ISC_R_FAILURE);

	if ((row = mysql_fetch_row(res)) == NULL) {
		mysql_free_result(res);
		return (ISC_R_FAILURE);
	}

	strcpy(zone_id, row[0]);
	mysql_free_result(res);

	/*
	 * See if we already have a transaction for this zone
	 */
	dlz_mutex_lock(&state->tx_mutex);
	for (txn = state->transactions; txn != NULL; txn = txn->next) {
		if (strcmp(txn->zone, zone) == 0) {
			if (state->log != NULL)
				state->log(ISC_LOG_ERROR,
				   "%s: transaction already "
				   "started for zone %s", modname, zone);
			dlz_mutex_unlock(&state->tx_mutex);
			return (ISC_R_FAILURE);
		}
	}

	/*
	 * Create new transaction
	 */
	newtx = (mysql_transaction_t *)
		malloc(sizeof(mysql_transaction_t));
	newtx->zone = strdup(zone);
	newtx->zone_id = strdup(zone_id);
	newtx->dbi = get_dbi(state);
	newtx->next = NULL;

	if (newtx->dbi == NULL) {
		result = ISC_R_FAILURE;
		goto cleanup;
	}

	result = db_execute(state, newtx->dbi, "START TRANSACTION");
	if (result != ISC_R_SUCCESS) {
		dlz_mutex_unlock(&newtx->dbi->mutex);
		goto cleanup;
	}

	/*
	 * Add this tx to front of list
	 */
	newtx->next = state->transactions;
	state->transactions = newtx;

	if (state->debug && (state->log != NULL))
		state->log(ISC_LOG_INFO, "%s: New tx %x", modname, newtx);

 cleanup:
	dlz_mutex_unlock(&state->tx_mutex);
	if (result == ISC_R_SUCCESS) {
		*versionp = (void *) newtx;
	} else {
		dlz_mutex_unlock(&state->tx_mutex);
		free(newtx->zone);
		free(newtx->zone_id);
		free(newtx);
	}

	return (result);
}

/*
 * End a transaction
 */
void
dlz_closeversion(const char *zone, isc_boolean_t commit,
		 void *dbdata, void **versionp)
{
	isc_result_t result;
	mysql_data_t *state = (mysql_data_t *)dbdata;
	mysql_transaction_t *txn = (mysql_transaction_t *)*versionp;
	mysql_transaction_t *txp;
	char *query;
	MYSQL_RES *res;
	MYSQL_ROW row;

	/*
	 * Find the transaction
	 */
	dlz_mutex_lock(&state->tx_mutex);
	if (state->transactions == txn) {
		/* Tx is first in list; remove it. */
		state->transactions = txn->next;
	} else {
		txp = state->transactions;
		while (txp != NULL) {
			if (txp->next != NULL) {
				if (txp->next == txn) {
					txp->next = txn->next;
					break;
				}
			}
			if (txp == txn) {
				txp = txn->next;
				break;
			}
			txp = txp->next;
		}
	}

	/*
	 * Tidy up
	 */
	dlz_mutex_unlock(&state->tx_mutex);
	*versionp = NULL;

	if (commit) {
		int oldsn = 0, newsn = 0;

		/*
		 * Find out the serial number of the zone out with the
		 * transaction so we can see if it has incremented or not
		 */
		query = build_query(state, txn->dbi, Q_GETSERIAL, zone);
		if (query == NULL && state->log != NULL) {
			state->log(ISC_LOG_ERROR,
				   "%s: unable to commit transaction %x "
				   "on zone %s: no memory",
				   modname, txn, zone);
			return;
		}

		res = db_query(state, txn->dbi, query);
		if (res != NULL) {
			while ((row = mysql_fetch_row(res)) != NULL)
				sscanf(row[0], "%d", &oldsn);
			mysql_free_result(res);
		}

		/*
		 * Commit the transaction to the database
		 */
		result = db_execute(state, txn->dbi, "COMMIT");
		if (result != ISC_R_SUCCESS && state->log != NULL) {
			state->log(ISC_LOG_INFO,
				   "%s: (%x) commit transaction on zone %s",
				   modname, txn, zone);
			return;
		}

		if (state->debug && state->log != NULL)
			state->log(ISC_LOG_INFO,
				   "%s: (%x) committing transaction "
				   "on zone %s",
				   modname, txn, zone);

		/*
		 * Now get the serial number again
		 */
		query = build_query(state, txn->dbi, Q_GETSERIAL, zone);
		res = db_query(state, txn->dbi, query);
		free(query);

		if (res != NULL) {
			while ((row = mysql_fetch_row(res)) != NULL)
				sscanf(row[0], "%d", &newsn);
			mysql_free_result(res);
		}

		/*
		 * Look to see if serial numbers have changed
		 */
		if (newsn > oldsn)
			notify(state, zone, newsn);
	} else {
		result = db_execute(state, txn->dbi, "ROLLBACK");
		if (state->debug && (state->log != NULL))
			state->log(ISC_LOG_INFO,
				   "%s: (%x) roll back transaction on zone %s",
				   modname, txn, zone);
	}

	/*
	 * Unlock the mutex for this txn
	 */
	dlz_mutex_unlock(&txn->dbi->mutex);

	/*
	 * Free up other structures
	 */
	free(txn->zone);
	free(txn->zone_id);
	free(txn);
}

/*
 * Configure a writeable zone
 */
#if DLZ_DLOPEN_VERSION < 3
isc_result_t
dlz_configure(dns_view_t *view, void *dbdata)
#else /* DLZ_DLOPEN_VERSION >= 3 */
isc_result_t
dlz_configure(dns_view_t *view, dns_dlzdb_t *dlzdb, void *dbdata)
#endif /* DLZ_DLOPEN_VERSION */
{
	mysql_data_t *state = (mysql_data_t *)dbdata;
	isc_result_t result;
	MYSQL_RES *res;
	MYSQL_ROW row;
	int count;

	/*
	 * Seed PRNG (used by Notify code)
	 */
	srand(getpid());

	if (state->debug && state->log != NULL)
		state->log(ISC_LOG_INFO, "%s: dlz_confgure", modname);

	if (state->writeable_zone == NULL) {
		if (state->log != NULL)
			state->log(ISC_LOG_ERROR,
				   "%s: no writeable_zone method available",
				   modname);
		return (ISC_R_FAILURE);
	}

	/*
	 * Get a list of Zones (ignore writeable column at this point)
	 */
	res = db_query(state, NULL, Q_GETZONES);
	if (res == NULL)
		return (ISC_R_FAILURE);

	count = 0;
	while ((row = mysql_fetch_row(res)) != NULL) {
		int sn;
		sscanf(row[1], "%d", &sn);
		notify(state, row[0], sn);
		result = state->writeable_zone(view,
#if DLZ_DLOPEN_VERSION >= 3
					       dlzdb,
#endif
					       row[0]);
		if (result != ISC_R_SUCCESS) {
			if (state->log != NULL)
				state->log(ISC_LOG_ERROR,
					   "%s: failed to configure zone %s",
					   modname, row[0]);
			mysql_free_result(res);
			return (result);
		}
		count++;
	}
	mysql_free_result(res);

	if (state->debug && state->log != NULL)
		state->log(ISC_LOG_INFO,
			   "%s: configured %d zones", modname, count);
	return (ISC_R_SUCCESS);
}

/*
 * Authorize a zone update
 */
isc_boolean_t
dlz_ssumatch(const char *signer, const char *name, const char *tcpaddr,
	     const char *type, const char *key, uint32_t keydatalen,
	     unsigned char *keydata, void *dbdata)
{
	mysql_data_t *state = (mysql_data_t *)dbdata;

	UNUSED(tcpaddr);
	UNUSED(type);
	UNUSED(keydatalen);
	UNUSED(keydata);
	UNUSED(key);

	if (state->debug && state->log != NULL)
		state->log(ISC_LOG_INFO,
			   "%s: allowing update of %s by key %s",
			   modname, name, signer);
	return (ISC_TRUE);
}

isc_result_t
dlz_addrdataset(const char *name, const char *rdatastr,
		void *dbdata, void *version)
{
	mysql_data_t *state = (mysql_data_t *)dbdata;
	mysql_transaction_t *txn = (mysql_transaction_t *)version;
	char *new_name, *query;
	mysql_record_t *record;
	isc_result_t result;

	if (txn == NULL)
		return (ISC_R_FAILURE);

	new_name = relname(name, txn->zone);
	if (new_name == NULL)
		return (ISC_R_NOMEMORY);

	if (state->debug && (state->log != NULL))
		state->log(ISC_LOG_INFO, "%s: add (%x) %s (as %s) %s",
			   modname, version, name, new_name, rdatastr);

	record = makerecord(state, new_name, rdatastr);
	free(new_name);
	if (record == NULL)
		return (ISC_R_FAILURE);

	/* Write out data to database */
	if (strcasecmp(record->type, "SOA") != 0) {
		query = build_query(state, txn->dbi, I_DATA,
				    txn->zone_id, record->name,
				    record->type, record->data,
				    record->ttl);
		if (query == NULL) {
			result = ISC_R_FAILURE;
			goto cleanup;
		}
		result = db_execute(state, txn->dbi, query);
		free(query);
	} else {
		/*
		 * This is an SOA record, so we update: it must exist,
		 * or we wouldn't have gotten this far.
		 * SOA: zone admin serial refresh retry expire min
		 */
		char sn[32];
		sscanf(record->data, "%*s %*s %31s %*s %*s %*s %*s", sn);
		query = build_query(state, txn->dbi, U_SERIAL, sn,
				    txn->zone_id);
		if (query == NULL) {
			result = ISC_R_FAILURE;
			goto cleanup;
		}
		result = db_execute(state, txn->dbi, query);
		free(query);
	}

 cleanup:
	free(record);
	return (result);
}

isc_result_t
dlz_subrdataset(const char *name, const char *rdatastr,
		void *dbdata, void *version)
{
	mysql_data_t *state = (mysql_data_t *)dbdata;
	mysql_transaction_t *txn = (mysql_transaction_t *)version;
	char *new_name, *query;
	mysql_record_t *record;
	isc_result_t result;

	if (txn == NULL)
		return (ISC_R_FAILURE);

	new_name = relname(name, txn->zone);
	if (new_name == NULL)
		return (ISC_R_NOMEMORY);

	if (state->debug && (state->log != NULL))
		state->log(ISC_LOG_INFO, "%s: sub (%x) %s %s",
			   modname, version, name, rdatastr);

	record = makerecord(state, new_name, rdatastr);
	free(new_name);
	if (record == NULL)
		return (ISC_R_FAILURE);
	/*
	 * If 'type' isn't 'SOA', delete the records
	 */
	if (strcasecmp(record->type, "SOA") == 0)
		result = ISC_R_SUCCESS;
	else {
		query = build_query(state, txn->dbi, D_RECORD,
				    txn->zone_id, record->name,
				    record->type, record->data,
				    record->ttl);
		if (query == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}

		result = db_execute(state, txn->dbi, query);
		free(query);
	}

 cleanup:
	free(record);
	return (result);
}

isc_result_t
dlz_delrdataset(const char *name, const char *type,
		void *dbdata, void *version)
{
	mysql_data_t *state = (mysql_data_t *)dbdata;
	mysql_transaction_t *txn = (mysql_transaction_t *)version;
	char *new_name, *query;
	isc_result_t result;

	if (txn == NULL)
		return (ISC_R_FAILURE);

	new_name = relname(name, txn->zone);
	if (new_name == NULL)
		return (ISC_R_NOMEMORY);

	if (state->debug && (state->log != NULL))
		state->log(ISC_LOG_INFO, "%s: del (%x) %s %s",
			   modname, version, name, type);

	query = build_query(state, txn->dbi, D_RRSET,
			    txn->zone_id, new_name, type);
	if (query == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	result = db_execute(state, txn->dbi, query);
	free(query);

 cleanup:
	free(new_name);
	return (result);
}
