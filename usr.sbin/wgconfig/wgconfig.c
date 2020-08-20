/*	$NetBSD: wgconfig.c,v 1.2 2020/08/20 21:31:26 riastradh Exp $	*/

/*
 * Copyright (C) Ryota Ozaki <ozaki.ryota@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: wgconfig.c,v 1.2 2020/08/20 21:31:26 riastradh Exp $");

#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_wg.h>

#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <errno.h>
#include <resolv.h>
#include <util.h>
#include <netdb.h>

#include <prop/proplib.h>

#define PROP_BUFFER_LEN	4096
#define KEY_LEN			32
#define KEY_BASE64_LEN		44

__dead static void
usage(void)
{
	const char *progname = getprogname();
#define P(str) fprintf(stderr, "\t%s <interface> %s\n", progname, str)

	fprintf(stderr, "Usage:\n");
	P("[show all]");
	P("show peer <peer name> [--show-preshared-key]");
	P("show private-key");
	P("set private-key <file path>");
	P("set listen-port <port>");
	P("add peer <peer name> <base64 public key>\n"
	"\t                     [--preshared-key=<file path>] [--endpoint=<ip>:<port>]\n"
	"\t                     [--allowed-ips=<ip1>/<cidr1>[,<ip2>/<cidr2>]...]");
	P("delete peer <peer name>");

	exit(EXIT_FAILURE);
#undef P
}

static const char *
format_key(prop_object_t key_prop)
{
	int error;
	unsigned char *key;
	size_t key_len;
	static char key_b64[KEY_BASE64_LEN + 1];
	static const char *none = "(none)";

	if (key_prop == NULL)
		return none;

	key = prop_data_data(key_prop);
	key_len = prop_data_size(key_prop);
	if (key_len != KEY_LEN)
		errx(EXIT_FAILURE, "invalid key len: %lu", key_len);
	error = b64_ntop(key, key_len, key_b64, KEY_BASE64_LEN + 1);
	if (error == -1)
		errx(EXIT_FAILURE, "b64_ntop failed");
	key_b64[KEY_BASE64_LEN] = '\0'; /* just in case */

	return key_b64;
}

static const char *
format_endpoint(prop_object_t endpoint_prop)
{
	int error;
	static char buf[INET6_ADDRSTRLEN];
	struct sockaddr_storage sockaddr;
	char *addr;
	size_t addr_len;

	addr = prop_data_data(endpoint_prop);
	addr_len = prop_data_size(endpoint_prop);
	memcpy(&sockaddr, addr, addr_len);

	error = sockaddr_snprintf(buf, sizeof(buf), "%a:%p",
	    (struct sockaddr *)&sockaddr);
	if (error == -1)
		err(EXIT_FAILURE, "sockaddr_snprintf failed");

	return buf;
}

static void
handle_allowed_ips(prop_dictionary_t peer, const char *prefix)
{
	prop_array_t allowedips;
	prop_object_iterator_t it;
	prop_dictionary_t allowedip;
	bool first = true;

	allowedips = prop_dictionary_get(peer, "allowedips");
	if (allowedips == NULL)
		return;

	printf("%sallowed-ips: ", prefix);

	it = prop_array_iterator(allowedips);
	while ((allowedip = prop_object_iterator_next(it)) != NULL) {
		prop_object_t prop_obj;
		uint8_t family;
		uint8_t cidr;
		char *addr;
		char ntopbuf[INET6_ADDRSTRLEN];
		const char *ntopret;

		prop_obj = prop_dictionary_get(allowedip, "family");
		if (prop_obj == NULL) {
			warnx("allowed-ip without family");
			continue;
		}

		family = (uint8_t)prop_number_unsigned_integer_value(prop_obj);

		prop_obj = prop_dictionary_get(allowedip, "cidr");
		if (prop_obj == NULL) {
			warnx("allowed-ip without cidr");
			continue;
		}
		cidr = (uint8_t)prop_number_unsigned_integer_value(prop_obj);

		prop_obj = prop_dictionary_get(allowedip, "ip");
		if (prop_obj == NULL) {
			warnx("allowed-ip without ip");
			continue;
		}

		addr = prop_data_data(prop_obj);
		ntopret = inet_ntop(family, addr, ntopbuf, sizeof(ntopbuf));
		if (ntopret == NULL)
			errx(EXIT_FAILURE, "inet_ntop failed");
		printf("%s%s/%u", first ? "" : ",", ntopbuf, cidr);
		first = false;
	}
	if (first)
		printf("(none)\n");
	else
		printf("\n");
}

static prop_dictionary_t
ioctl_get(const char *interface)
{
	int error = 0;
	struct ifdrv ifd;
	int sock;
	char *buf;
	prop_dictionary_t prop_dict;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (error == -1)
		err(EXIT_FAILURE, "socket");
	buf = malloc(PROP_BUFFER_LEN);
	if (buf == NULL)
		errx(EXIT_FAILURE, "malloc failed");

	strlcpy(ifd.ifd_name, interface, sizeof(ifd.ifd_name));
	ifd.ifd_cmd = 0;
	ifd.ifd_data = buf;
	ifd.ifd_len = PROP_BUFFER_LEN;

	error = ioctl(sock, SIOCGDRVSPEC, &ifd);
	if (error == -1)
		err(EXIT_FAILURE, "ioctl(SIOCGDRVSPEC)");

	prop_dict = prop_dictionary_internalize(buf);
	if (prop_dict == NULL)
		errx(EXIT_FAILURE, "prop_dictionary_internalize failed");

	free(buf);
	close(sock);

	return prop_dict;
}

static void
show_peer(prop_dictionary_t peer, const char *prefix, bool show_psk)
{
	prop_object_t prop_obj;

	prop_obj = prop_dictionary_get(peer, "public_key");
	if (prop_obj == NULL) {
		warnx("peer without public-key");
		return;
	}
	printf("%spublic-key: %s\n", prefix, format_key(prop_obj));

	prop_obj = prop_dictionary_get(peer, "endpoint");
	if (prop_obj == NULL)
		printf("%sendpoint: (none)\n", prefix);
	else
		printf("%sendpoint: %s\n", prefix, format_endpoint(prop_obj));

	if (show_psk) {
		prop_obj = prop_dictionary_get(peer, "preshared_key");
		printf("%spreshared-key: %s\n", prefix, format_key(prop_obj));
	} else {
		printf("%spreshared-key: (hidden)\n", prefix);
	}

	handle_allowed_ips(peer, prefix);

	prop_obj = prop_dictionary_get(peer, "last_handshake_time_sec");
	if (prop_obj != NULL) {
		uint64_t sec = prop_number_unsigned_integer_value(prop_obj);
		printf("%slatest-handshake: %"PRIu64"\n", prefix, sec);
	} else
		printf("%slatest-handshake: (none)\n", prefix);
#if 0
	prop_obj = prop_dictionary_get(peer, "last_handshake_time_nsec");
#endif
}

static int
cmd_show_all(const char *interface, int argc, char *argv[])
{
	prop_dictionary_t prop_dict;
	prop_object_t prop_obj;

	prop_dict = ioctl_get(interface);

	printf("interface: %s\n", interface);

#if 0
	prop_obj = prop_dictionary_get(prop_dict, "private_key");
	printf("\tprivate-key: %s\n", format_key(prop_obj));
#else
	printf("\tprivate-key: (hidden)\n");
#endif

	prop_obj = prop_dictionary_get(prop_dict, "listen_port");
	if (prop_obj != NULL) {
		uint64_t port = prop_number_unsigned_integer_value(prop_obj);
		if (port != (uint64_t)(uint16_t)port)
			errx(EXIT_FAILURE, "invalid port: %" PRIu64, port);
		printf("\tlisten-port: %u\n", (uint16_t)port);
	} else {
		printf("\tlisten-port: (none)\n");
	}

	prop_array_t peers = prop_dictionary_get(prop_dict, "peers");
	if (peers == NULL)
		return EXIT_SUCCESS;

	prop_object_iterator_t it = prop_array_iterator(peers);
	prop_dictionary_t peer;
	while ((peer = prop_object_iterator_next(it)) != NULL) {
		prop_obj = prop_dictionary_get(peer, "name");
		if (prop_obj != NULL) {
			const char *name = prop_string_cstring_nocopy(prop_obj);
			printf("\tpeer: %s\n", name);
		} else
			printf("\tpeer: (none)\n");

		show_peer(peer, "\t\t", false);
	}

	return EXIT_SUCCESS;
}

static int
cmd_show_peer(const char *interface, int argc, char *argv[])
{
	prop_dictionary_t prop_dict;
	const char *target;
	const char *opt = "--show-preshared-key";
	bool show_psk = false;

	if (argc != 1 && argc != 2)
		usage();
	target = argv[0];
	if (argc == 2) {
		if (strncmp(argv[1], opt, strlen(opt)) != 0)
			usage();
		show_psk = true;
	}

	prop_dict = ioctl_get(interface);

	prop_array_t peers = prop_dictionary_get(prop_dict, "peers");
	if (peers == NULL)
		return EXIT_SUCCESS;

	prop_object_iterator_t it = prop_array_iterator(peers);
	prop_dictionary_t peer;
	while ((peer = prop_object_iterator_next(it)) != NULL) {
		prop_object_t prop_obj;
		prop_obj = prop_dictionary_get(peer, "name");
		if (prop_obj == NULL)
			continue;
		const char *name = prop_string_cstring_nocopy(prop_obj);
		if (strcmp(name, target) == 0) {
			printf("peer: %s\n", name);
			show_peer(peer, "\t", show_psk);
			break;
		}
	}

	return EXIT_SUCCESS;
}

static int
cmd_show_private_key(const char *interface, int argc, char *argv[])
{
	prop_dictionary_t prop_dict;
	prop_object_t prop_obj;

	prop_dict = ioctl_get(interface);

	prop_obj = prop_dictionary_get(prop_dict, "private_key");
	printf("private-key: %s\n", format_key(prop_obj));

	return EXIT_SUCCESS;
}

static void
ioctl_set(const char *interface, int cmd, char *propstr)
{
	int error;
	struct ifdrv ifd;
	int sock;

	strlcpy(ifd.ifd_name, interface, sizeof(ifd.ifd_name));
	ifd.ifd_cmd = cmd;
	ifd.ifd_data = propstr;
	ifd.ifd_len = strlen(propstr);
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	error = ioctl(sock, SIOCSDRVSPEC, &ifd);
	if (error == -1)
		err(EXIT_FAILURE, "ioctl(SIOCSDRVSPEC): cmd=%d", cmd);
	close(sock);
}

static void
base64_decode(const char keyb64buf[KEY_BASE64_LEN + 1],
    unsigned char keybuf[KEY_LEN])
{
	int ret;

	ret = b64_pton(keyb64buf, keybuf, KEY_LEN);
	if (ret == -1)
		errx(EXIT_FAILURE, "b64_pton failed");
}

static void
read_key(const char *path, unsigned char keybuf[KEY_LEN])
{
	FILE *fp;
	char keyb64buf[KEY_BASE64_LEN + 1];
	size_t n;

	fp = fopen(path, "r");
	if (fp == NULL)
		err(EXIT_FAILURE, "fopen");

	n = fread(keyb64buf, 1, KEY_BASE64_LEN, fp);
	if (n != KEY_BASE64_LEN)
		errx(EXIT_FAILURE, "base64 key len is short: %lu", n);
	keyb64buf[KEY_BASE64_LEN] = '\0';

	base64_decode(keyb64buf, keybuf);
}

static int
cmd_set_private_key(const char *interface, int argc, char *argv[])
{
	unsigned char keybuf[KEY_LEN];

	if (argc != 1)
		usage();

	read_key(argv[0], keybuf);

	prop_dictionary_t prop_dict;
	prop_dict = prop_dictionary_create();
	prop_data_t privkey = prop_data_create_data(keybuf, sizeof(keybuf));
	prop_dictionary_set(prop_dict, "private_key", privkey);
	prop_object_release(privkey);

	char *buf = prop_dictionary_externalize(prop_dict);
	if (buf == NULL)
		err(EXIT_FAILURE, "prop_dictionary_externalize failed");
	ioctl_set(interface, WG_IOCTL_SET_PRIVATE_KEY, buf);

	return EXIT_SUCCESS;
}

static uint16_t
strtouint16(const char *str)
{
	char *ep;
	long val;

	errno = 0;
	val = strtol(str, &ep, 10);
	if (ep == str)
		errx(EXIT_FAILURE, "strtol: not a number");
	if (*ep != '\0')
		errx(EXIT_FAILURE, "strtol: trailing garbage");
	if (errno != 0)
		err(EXIT_FAILURE, "strtol");
	if (val < 0 || val > USHRT_MAX)
		errx(EXIT_FAILURE, "out of range");

	return (uint16_t)val;
}

static int
cmd_set_listen_port(const char *interface, int argc, char *argv[])
{
	uint16_t port;

	if (argc != 1)
		usage();

	port = strtouint16(argv[0]);
	if (port == 0)
		errx(EXIT_FAILURE, "port 0 is not allowed");

	prop_dictionary_t prop_dict;
	prop_dict = prop_dictionary_create();
	prop_number_t prop_port = prop_number_create_unsigned_integer(port);
	prop_dictionary_set(prop_dict, "listen_port", prop_port);
	prop_object_release(prop_port);

	char *buf = prop_dictionary_externalize(prop_dict);
	if (buf == NULL)
		err(EXIT_FAILURE, "prop_dictionary_externalize failed");
	ioctl_set(interface, WG_IOCTL_SET_LISTEN_PORT, buf);

	return EXIT_SUCCESS;
}

static void
handle_option_endpoint(const char *_addr_port, prop_dictionary_t prop_dict)
{
	int error;
	prop_data_t prop_addr;
	char *port;
	struct addrinfo hints, *res;
	char *addr_port, *addr;

	addr = addr_port = strdup(_addr_port);

	if (addr_port[0] == '[') {
		/* [<ipv6>]:<port> */
		/* Accept [<ipv4>]:<port> too, but it's not a big deal. */
		char *bracket, *colon;
		if (strlen(addr_port) < strlen("[::]:0"))
			errx(EXIT_FAILURE, "invalid endpoint format");
		addr = addr_port + 1;
		bracket = strchr(addr, ']');
		if (bracket == NULL)
			errx(EXIT_FAILURE, "invalid endpoint format");
		*bracket = '\0';
		colon = bracket + 1;
		if (*colon != ':')
			errx(EXIT_FAILURE, "invalid endpoint format");
		*colon = '\0';
		port = colon + 1;
	} else {
		char *colon, *tmp;
		colon = strchr(addr_port, ':');
		if (colon == NULL)
			errx(EXIT_FAILURE, "no ':' found in endpoint");
		tmp = strchr(colon + 1, ':');
		if (tmp != NULL) {
			/* <ipv6>:<port> */
			/* Assume the last colon is a separator */
			char *last_colon = tmp;
			while ((tmp = strchr(tmp + 1, ':')) != NULL)
				last_colon = tmp;
			colon = last_colon;
			*colon = '\0';
			port = colon + 1;
		} else {
			/* <ipv4>:<port> */
			*colon = '\0';
			port = colon + 1;
		}
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_NUMERICHOST;
	error = getaddrinfo(addr, port, &hints, &res);
	if (error != 0)
		err(EXIT_FAILURE, "getaddrinfo");

	prop_addr = prop_data_create_data(res->ai_addr, res->ai_addrlen);
	prop_dictionary_set(prop_dict, "endpoint", prop_addr);
	prop_object_release(prop_addr);

	freeaddrinfo(res);
	free(addr_port);
}

static void
handle_option_allowed_ips(const char *_allowed_ips, prop_dictionary_t prop_dict)
{
	prop_array_t allowedips;
	int i;
	char *allowed_ips, *ip;

	allowed_ips = strdup(_allowed_ips);

	allowedips = prop_array_create();
	i = 0;
	while ((ip = strsep(&allowed_ips, ",")) != NULL) {
		prop_dictionary_t prop_allowedip;
		prop_allowedip = prop_dictionary_create();
		uint16_t cidr;
		char *cidrp;
		struct addrinfo hints, *res;
		int error;

		cidrp = strchr(ip, '/');
		if (cidrp == NULL)
			errx(EXIT_FAILURE, "no '/' found in allowed-ip");
		*cidrp = '\0';
		cidrp++;

		cidr = strtouint16(cidrp);

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_NUMERICHOST;
		error = getaddrinfo(ip, 0, &hints, &res);
		if (error != 0)
			err(EXIT_FAILURE, "getaddrinfo");

		sa_family_t family = res->ai_addr->sa_family;
		prop_number_t prop_family = prop_number_create_unsigned_integer(family);
		prop_dictionary_set(prop_allowedip, "family", prop_family);
		prop_object_release(prop_family);

		prop_data_t addr;
		if (family == AF_INET) {
			struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
			addr = prop_data_create_data(&sin->sin_addr, sizeof(sin->sin_addr));
		} else if (family == AF_INET6) {
			struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)res->ai_addr;
			addr = prop_data_create_data(&sin6->sin6_addr, sizeof(sin6->sin6_addr));
		} else
			errx(EXIT_FAILURE, "invalid family: %d", family);
		prop_dictionary_set(prop_allowedip, "ip", addr);
		prop_object_release(addr);

		prop_number_t prop_cidr = prop_number_create_unsigned_integer(cidr);
		prop_dictionary_set(prop_allowedip, "cidr", prop_cidr);
		prop_object_release(prop_cidr);

		freeaddrinfo(res);
		prop_array_set(allowedips, i, prop_allowedip);
		i++;
	}
	prop_dictionary_set(prop_dict, "allowedips", allowedips);

	free(allowed_ips);
}

static void
handle_option_preshared_key(const char *path, prop_dictionary_t prop_dict)
{
	unsigned char keybuf[KEY_LEN];
	prop_data_t psk;

	read_key(path, keybuf);
	psk = prop_data_create_data(keybuf, sizeof(keybuf));
	prop_dictionary_set(prop_dict, "preshared_key", psk);
	prop_object_release(psk);
}

static const struct option {
	const char	*option;
	void		(*func)(const char *, prop_dictionary_t);
} options[] = {
	{"--endpoint=",		handle_option_endpoint},
	{"--allowed-ips=",	handle_option_allowed_ips},
	{"--preshared-key=",	handle_option_preshared_key},
};

static void
handle_options(int argc, char *argv[], prop_dictionary_t prop_dict)
{

	while (argc > 0) {
		for (size_t i = 0; i < __arraycount(options); i++) {
			const struct option *opt = &options[i];
			size_t optlen = strlen(opt->option);
			if (strncmp(argv[0], opt->option, optlen) == 0) {
				opt->func(argv[0] + optlen, prop_dict);
				break;
			}
		}
		argc -= 1;
		argv += 1;
	}

	if (argc != 0)
		usage();
}

static int
cmd_add_peer(const char *interface, int argc, char *argv[])
{
	const char *name;
	unsigned char keybuf[KEY_LEN];

	if (argc < 2)
		usage();

	prop_dictionary_t prop_dict;
	prop_dict = prop_dictionary_create();

	name = argv[0];
	if (strlen(name) > WG_PEER_NAME_MAXLEN)
		errx(EXIT_FAILURE, "peer name too long");
	if (strnlen(argv[1], KEY_BASE64_LEN + 1) != KEY_BASE64_LEN)
		errx(EXIT_FAILURE, "invalid public-key length: %lu", strlen(argv[1]));
	base64_decode(argv[1], keybuf);

	prop_string_t prop_name = prop_string_create_cstring(name);
	prop_dictionary_set(prop_dict, "name", prop_name);
	prop_object_release(prop_name);

	prop_data_t pubkey = prop_data_create_data(keybuf, sizeof(keybuf));
	prop_dictionary_set(prop_dict, "public_key", pubkey);
	prop_object_release(pubkey);

	argc -= 2;
	argv += 2;

	handle_options(argc, argv, prop_dict);

	char *buf = prop_dictionary_externalize(prop_dict);
	if (buf == NULL)
		err(EXIT_FAILURE, "prop_dictionary_externalize failed");
	ioctl_set(interface, WG_IOCTL_ADD_PEER, buf);

	return EXIT_SUCCESS;
}

static int
cmd_delete_peer(const char *interface, int argc, char *argv[])
{
	const char *name;

	if (argc != 1)
		usage();

	prop_dictionary_t prop_dict;
	prop_dict = prop_dictionary_create();

	name = argv[0];
	if (strlen(name) > WG_PEER_NAME_MAXLEN)
		errx(EXIT_FAILURE, "peer name too long");

	prop_string_t prop_name = prop_string_create_cstring(name);
	prop_dictionary_set(prop_dict, "name", prop_name);
	prop_object_release(prop_name);

	char *buf = prop_dictionary_externalize(prop_dict);
	if (buf == NULL)
		err(EXIT_FAILURE, "prop_dictionary_externalize failed");
	ioctl_set(interface, WG_IOCTL_DELETE_PEER, buf);

	return EXIT_SUCCESS;
}

static const struct command {
	const char	*command;
	const char	*target;
	int		(*func)(const char *, int, char **);
} commands[] = {
	{"show",	"all",		cmd_show_all},
	{"show",	"peer",		cmd_show_peer},
	{"show",	"private-key",	cmd_show_private_key},
	{"set",		"private-key",	cmd_set_private_key},
	{"set",		"listen-port",	cmd_set_listen_port},
	{"add",		"peer",		cmd_add_peer},
	{"delete",	"peer",		cmd_delete_peer},
};

int
main(int argc, char *argv[])
{
	const char *interface;
	const char *command;
	const char *target;

	if (argc < 2 ||
	    strcmp(argv[1], "-h") == 0 ||
	    strcmp(argv[1], "-?") == 0 ||
	    strcmp(argv[1], "--help") == 0) {
		usage();
	}

	interface = argv[1];
	if (strlen(interface) > IFNAMSIZ)
		errx(EXIT_FAILURE, "interface name too long");
	if (argc == 2) {
		return cmd_show_all(interface, 0, NULL);
	}
	if (argc < 4) {
		usage();
	}
	command = argv[2];
	target = argv[3];

	argc -= 4;
	argv += 4;

	for (size_t i = 0; i < __arraycount(commands); i++) {
		const struct command *cmd = &commands[i];
		if (strncmp(command, cmd->command, strlen(cmd->command)) == 0 &&
		    strncmp(target, cmd->target, strlen(cmd->target)) == 0) {
			return cmd->func(interface, argc, argv);
		}
	}

	usage();
}
