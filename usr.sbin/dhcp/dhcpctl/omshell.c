/* omshell.c

   Examine and modify omapi objects. */

/*
 * Copyright (c) 2001 Internet Software Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about the Internet Software Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <isc/result.h>
#include "dhcpctl.h"
#include "dhcpd.h"

/* Fixups */
isc_result_t find_class (struct class **c, const char *n, const char *f, int l)
{
	return 0;
}
int parse_allow_deny (struct option_cache **oc, struct parse *cfile, int flag)
{
	return 0;
}
void dhcp (struct packet *packet) { }
void bootp (struct packet *packet) { }
int check_collection (struct packet *p, struct lease *l, struct collection *c)
{
	return 0;
}
void classify (struct packet *packet, struct class *class) { }

static void usage (char *s) {
	fprintf (stderr,
		 "Usage: %s [-n <username>] [-p <password>] "
		 "[-a <algorithm>] [-P <port>]\n", s);
	exit (1);
}

static void check (isc_result_t status, const char *func) {
	if (status != ISC_R_SUCCESS) {
		fprintf (stderr, "%s: %s\n", func, isc_result_totext (status));
		exit (1);
	}
}

int main (int argc, char **argv, char **envp)
{
	isc_result_t status, waitstatus;
	dhcpctl_handle connection;
	dhcpctl_handle authenticator;
	dhcpctl_handle oh;
	dhcpctl_data_string cid, ip_addr;
	dhcpctl_data_string result, groupname, identifier;
	const char *name = 0, *pass = 0, *algorithm = "hmac-md5";
	int i, j;
	int port = 7911;
	const char *server = "127.0.0.1";
	struct parse *cfile;
	enum dhcp_token token;
	const char *val;
	char buf[1024];
	char s1[1024];

	for (i = 1; i < argc; i++) {
		if (!strcmp (argv[i], "-n")) {
			if (++i == argc)
				usage(argv[0]);
			name = argv[i];
		} else if (!strcmp (argv[i], "-p")) {
			if (++i == argc)
				usage(argv[0]);
			pass = argv[i];
		} else if (!strcmp (argv[i], "-a")) {
			if (++i == argc)
				usage(argv[0]);
			algorithm = argv[i];
		} else if (!strcmp (argv[i], "-s")) {
			if (++i == argc)
				usage(argv[0]);
			server = argv[i];
		} else if (!strcmp (argv[i], "-P")) {
			if (++i == argc)
				usage(argv[0]);
			port = atoi (argv[i]);
		} else {
			usage(argv[0]);
		}
	}

	if ((name || pass) && !(name && pass))
		usage(argv[0]);

	status = dhcpctl_initialize ();
	if (status != ISC_R_SUCCESS) {
		fprintf (stderr, "dhcpctl_initialize: %s\n",
			 isc_result_totext (status));
		exit (1);
	}

	authenticator = dhcpctl_null_handle;

	if (name) {
		status = dhcpctl_new_authenticator (&authenticator,
						    name, algorithm, pass,
						    strlen (pass) + 1);
		if (status != ISC_R_SUCCESS) {
			fprintf (stderr, "Cannot create authenticator: %s\n",
				 isc_result_totext (status));
			exit (1);
		}
	}

	memset (&connection, 0, sizeof connection);
	status = dhcpctl_connect (&connection, server, port, authenticator);
	if (status != ISC_R_SUCCESS) {
		fprintf (stderr, "dhcpctl_connect: %s\n",
			 isc_result_totext (status));
		exit (1);
	}

	memset (&oh, 0, sizeof oh);

	do {
		printf ("obj: ");
		if (oh == NULL) {
			printf ("<null>\n");
		} else {
			dhcpctl_remote_object_t *r =
				(dhcpctl_remote_object_t *)oh;
			omapi_generic_object_t *g =
				(omapi_generic_object_t *)(r -> inner);

			if (r -> rtype -> type != omapi_datatype_string) {
				printf ("?\n");
			} else {
				printf ("%.*s\n",
					(int)(r -> rtype -> u . buffer . len),
					r -> rtype -> u . buffer . value);
			}

			for (i = 0; i < g -> nvalues; i++) {
				omapi_value_t *v = g -> values [i];

				printf ("%.*s = ", (int)v -> name -> len,
					v -> name -> value);

				switch (v -> value -> type) {
				case omapi_datatype_int:
					printf ("%d\n",
						v -> value -> u . integer);
					break;

				case omapi_datatype_string:
					printf ("\"%.*s\"\n",
						(int)v -> value -> u.buffer.len,
						v -> value -> u.buffer.value);
					break;

				case omapi_datatype_data:
					printf ("%s\n",
						print_hex_1
						(v -> value -> u.buffer.len,
						 v -> value -> u.buffer.value,
						 60));
					break;

				case omapi_datatype_object:
					printf ("<obj>\n");
					break;
				}
			}
		}

		fputs ("> ", stdout);
		fflush (stdout);
		if (fgets (buf, sizeof(buf), stdin) == NULL)
			break;

		status = new_parse (&cfile, 0, buf, strlen(buf), "<STDIN>");
		check(status, "new_parse()");

		token = next_token (&val, (unsigned *)0, cfile);
		switch (token) {
		      default:
			parse_warn (cfile, "unknown token: %s", val);
			break;

		      case END_OF_FILE:
			break;

		      case TOKEN_HELP:
		      case '?':
			printf ("Commands:\n");
			printf ("  new <object-type>\n");
			printf ("  set <name> = <value>\n");
			printf ("  create\n");
			printf ("  open\n");
			break;

		      case TOKEN_NEW:
			token = next_token (&val, (unsigned *)0, cfile);
			if ((!is_identifier (token) && token != STRING) ||
			    next_token (NULL,
					(unsigned *)0, cfile) != END_OF_FILE)
			{
				printf ("usage: new <object-type>\n");
				break;
			}

			if (oh) {
				printf ("an object is already open.\n");
				break;
			}

			status = dhcpctl_new_object (&oh, connection, val);
			if (status != ISC_R_SUCCESS) {
				printf ("can't create object: %s\n",
					isc_result_totext (status));
				break;
			}

			break;

		      case TOKEN_CLOSE:
			if (next_token (NULL,
					(unsigned *)0, cfile) != END_OF_FILE) {
				printf ("usage: close\n");
			}

			omapi_object_dereference (&oh, MDL);

			break;

		      case TOKEN_SET:
			token = next_token (&val, (unsigned *)0, cfile);

			if ((!is_identifier (token) && token != STRING) ||
			    next_token (NULL, (unsigned *)0, cfile) != '=')
			{
				printf ("usage: set <name> = <value>\n");
				break;
			}

			if (oh == NULL) {
				printf ("no open object.\n");
				break;
			}

			s1[0] = '\0';
			strncat (s1, val, sizeof(s1)-1);

			token = next_token (&val, (unsigned *)0, cfile);
			switch (token) {
			case STRING:
				dhcpctl_set_string_value (oh, val, s1);
				break;

			case NUMBER:
				dhcpctl_set_int_value (oh, atoi (val), s1);
				break;

			default:
				printf ("invalid value.\n");
			}

			break;

		      case TOKEN_CREATE:
		      case TOKEN_OPEN:
			if (next_token (NULL,
					(unsigned *)0, cfile) != END_OF_FILE) {
				printf ("usage: %s\n", val);
			}

			i = 0;
			if (token == TOKEN_CREATE)
				i = DHCPCTL_CREATE | DHCPCTL_EXCL;

			status = dhcpctl_open_object (oh, connection, i);
			if (status == ISC_R_SUCCESS)
				status = dhcpctl_wait_for_completion
					(oh, &waitstatus);
			if (status == ISC_R_SUCCESS)
				status = waitstatus;
			if (status != ISC_R_SUCCESS) {
				printf ("can't open object: %s\n",
					isc_result_totext (status));
				break;
			}

			break;

		      case UPDATE:
			if (next_token (NULL, (unsigned *)0,
					cfile) != END_OF_FILE) {
				printf ("usage: %s\n", val);
			}

			status = dhcpctl_object_update(connection, oh);
			if (status == ISC_R_SUCCESS)
				status = dhcpctl_wait_for_completion
					(oh, &waitstatus);
			if (status == ISC_R_SUCCESS)
				status = waitstatus;
			if (status != ISC_R_SUCCESS) {
				printf ("can't update object: %s\n",
					isc_result_totext (status));
				break;
			}

			break;
		}
	} while (1);

	exit (0);
}
