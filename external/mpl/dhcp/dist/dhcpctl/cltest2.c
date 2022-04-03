/*	$NetBSD: cltest2.c,v 1.2 2022/04/03 01:10:58 christos Exp $	*/

/* cltest2.c

   Example program that uses the dhcpctl library. */

/*
 * Copyright (C) 2020-2022 Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 * This software was contributed to Internet Systems Consortium
 * by Brian Murrell.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: cltest2.c,v 1.2 2022/04/03 01:10:58 christos Exp $");

#include "config.h"

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "omapip/result.h"
#include "dhcpctl.h"
#include "dhcpd.h"

int main (int, char **);

static void usage (char *s) {
	fprintf (stderr,
		 "Usage: %s [-s <server ip>] [-p <port>]", s);
	exit (1);
}

static void fail_on_error(isc_result_t status, const char* message) {
	if (status != ISC_R_SUCCESS) {
		fprintf (stderr, "%s: %s\n",
			 message, isc_result_totext (status));
		exit (1);
	}
}

isc_result_t wait_with_retry(dhcpctl_handle handle, struct timeval* timeout, int retries);

void print_object(char *msg, dhcpctl_handle handle);

/* Simple test program that exercises dhcpctl calls as follows:
 *
 * 1. Connect to the given server
 * 2. Create a local host object with hostname "cltest2.host"
 * 3. Attempt to open the remote host object
 * 4. If the host does not exist, add a client id and create it
 * 5. Disconnect
 * 6. Reconnect
 * 7. Refresh the host object
 * 8. Disconnect
 * 9. Time connect
 * 10. Time connect retry
 *
 * Note that this program tests dhcpctl_timed_wait_for_completion() by calling
 * it with extremely small timeouts.
*/

int main (argc, argv)
	int argc;
	char **argv;
{
	isc_result_t status;
	dhcpctl_handle connection;
	dhcpctl_handle host;
    char* ip_address = "127.0.0.1";
    int port = 7911;
    char* hostname = "cltest2.host";
    struct timeval timeout;
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp (argv[i], "-s")) {
            ip_address = argv[i];
		} else if (!strcmp (argv [i], "-p")) {
            port=atoi(argv[i]);
		} else if (argv[i][0] == '-') {
			usage(argv[0]);
		}
	}

    /* Initialize dhcpctl */
	status = dhcpctl_initialize ();
	fail_on_error(status ,"can't initialize dhcpctl");

    /* Connect */
	connection = 0;
	status = dhcpctl_connect (&connection, ip_address, port, 0);
	fail_on_error(status ,"connect failed");

    /* Create the host object */
	host = 0;
	status = dhcpctl_new_object (&host, connection, "host");
	fail_on_error(status ,"new oject failed");

	status = dhcpctl_set_string_value (host, hostname, "name");
	fail_on_error(status ,"cant set host name");

    /* Attempt to open the object */
	status = dhcpctl_open_object (host, connection, 0);
    timeout.tv_sec = 0;
    timeout.tv_usec = 20;
    status = wait_with_retry(host, &timeout, 2);
    switch (status) {
        case ISC_R_NOTFOUND:
            /* Host doesn't exist add it. We set an id so the create will be valid. */
	        status = dhcpctl_set_string_value (host, "abcdefg", "dhcp-client-identifier");
	        fail_on_error(status ,"can't set client id");

		    status = dhcpctl_open_object (host, connection,
					                      DHCPCTL_CREATE | DHCPCTL_EXCL);
		    fail_on_error(status, "open(create) failed");

            status = wait_with_retry(host, &timeout, 2);
		    fail_on_error(status, "wait after open(create)");

            print_object("Host created", host);
            break;

        case ISC_R_SUCCESS:
            print_object("Host exists", host);
            break;

        default:
	        fail_on_error(status, "initial open failed, waiting for completion");
            break;
    }

	/* Now we'll test disconnect */
	status = dhcpctl_disconnect(&connection, 0);
	fail_on_error(status, "can't disconnect");

    /* Reconnect */
	status = dhcpctl_connect (&connection, ip_address, port, 0);
	fail_on_error(status ,"can't reconnect");

    /* Refresh the object */
    status = dhcpctl_object_refresh (connection, host);
    fail_on_error(status , "can't refresh");

    status = wait_with_retry(host, &timeout, 2);
    fail_on_error(status , "wait after refresh failed");

    print_object("After reconnect/refresh", host);

	/* Now we'll disconnect */
	status = dhcpctl_disconnect(&connection, 0);
	fail_on_error(status, "can't disconnect");

    /* Try a timed connect */
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;
	status = dhcpctl_timed_connect (&connection, ip_address, port, 0, &timeout);

    /* Try again if we time out */
    if (status == ISC_R_TIMEDOUT) {
        printf ("Retry timed connect\n");
        timeout.tv_sec = 10;
	    status = dhcpctl_timed_connect (&connection, ip_address, port, 0,
                                        &timeout);
    }

    fail_on_error(status ,"can't reconnect");

    /* Lastly we'll disconnect to clean up */
	status = dhcpctl_disconnect(&connection, 0);
    fail_on_error(status ,"can't disconnect");

	exit (0);
}

/* Function to call and optionally retry dhcp_timed_wait_for_completion() */
isc_result_t wait_with_retry(dhcpctl_handle handle, struct timeval* timeout,  int retries) {
	isc_result_t status;
	isc_result_t waitstatus;
    struct timeval use_timeout;

    if (timeout) {
        use_timeout.tv_sec = timeout->tv_sec;
        use_timeout.tv_usec = timeout->tv_usec;
    } else {
        retries = 0;
    }

    int tries = 0;
    do {
        if (tries++) {
            printf ("wait retry #%d\n", tries);
            /* Set the timeout value to 30 secs */
            use_timeout.tv_sec = 30;
            use_timeout.tv_usec = 0;
        }

        // Call timed wait.
	    status = dhcpctl_timed_wait_for_completion (handle, &waitstatus,
                                                    (timeout ? &use_timeout: 0));
        if (status == ISC_R_SUCCESS) {
            return(waitstatus);
        }

        if (status != ISC_R_TIMEDOUT) {
            fprintf (stderr, "timed wait failed: %s\n", isc_result_totext (status));
            exit (1);
        }
   } while (--retries > 0);

    return (ISC_R_TIMEDOUT);
}

/* Function to print out the values contained in an object. Largely
 * stolen from omshell.c */
void print_object(char* msg, dhcpctl_handle handle) {
    dhcpctl_remote_object_t *r = (dhcpctl_remote_object_t *)handle;
    omapi_generic_object_t *object = (omapi_generic_object_t *)(r->inner);
    char hex_buf[4096];
    int i;

    printf ("%s:\n",msg);
    for (i = 0; i < object->nvalues; i++) {
        omapi_value_t *v = object->values[i];

        if (!object->values[i])
            continue;

        printf ("\t%.*s = ", (int)v->name->len, v->name->value);

        if (!v->value) {
            printf ("<null>\n");
            continue;
        }

        switch (v->value->type) {
        case omapi_datatype_int:
            printf ("%d\n", v->value->u.integer);
            break;

        case omapi_datatype_string:
            printf ("\"%.*s\"\n", (int)v->value->u.buffer.len,
                    v->value->u.buffer.value);
            break;

        case omapi_datatype_data:
            print_hex_or_string(v->value->u.buffer.len,
						        v->value->u.buffer.value,
                                sizeof(hex_buf), hex_buf);
            printf("%s\n", hex_buf);
            break;

        case omapi_datatype_object:
            printf ("<obj>\n");
            break;
        }
    }
}

/* Dummy functions to appease linker */
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

#ifdef DHCPv6
void dhcpv6(struct packet *packet) { }

#ifdef DHCP4o6
isc_result_t dhcpv4o6_handler(omapi_object_t *h)
{
	return ISC_R_NOTIMPLEMENTED;
}
#endif /* DHCP4o6 */
#endif /* DHCPv6 */

int check_collection (struct packet *p, struct lease *l, struct collection *c)
{
	return 0;
}
void classify (struct packet *packet, struct class *class) { }

isc_result_t dhcp_set_control_state (control_object_state_t oldstate,
				     control_object_state_t newstate)
{
	return ISC_R_SUCCESS;
}

