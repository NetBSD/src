/* $NetBSD: main.c,v 1.6.2.1 2013/01/16 05:34:09 yamt Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
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

#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include "ldp.h"
#include "ldp_command.h"
#include "socketops.h"
#include "tlv.h"
#include "pdu.h"
#include "fsm.h"
#include "ldp_errors.h"
#include "mpls_interface.h"
#include "conffile.h"

extern int ls;		/* TCP listening socket */
extern int dont_catch;
extern int command_port;
extern int command_socket;

extern int debug_f, warn_f, syslog_f;

extern struct sockaddr mplssockaddr;
extern struct in_addr conf_ldp_id;

void print_usage(char *myself)
{
	printf("\nUsage: %s [-DdfhW] [-c config_file] [-p port]\n\n", myself);
}

int 
main(int argc, char *argv[])
{
	int ch, forkres, dontfork = 0, cpf;
	char conffile[PATH_MAX + 1];

	strlcpy(conffile, CONFFILE, sizeof(conffile));
	while((ch = getopt(argc, argv, "c:dDfhp:W")) != -1)
		switch(ch) {
		case 'c':
			strlcpy(conffile, optarg, sizeof(conffile));
			break;
		case 'D':
			debug_f = 1;
			break;
		case 'd':
			dont_catch = 1;
			break;
		case 'f':
			dontfork = 1;
			break;
		case 'p':
			if ((command_port = atoi(optarg)) < 1) {
				print_usage(argv[0]);
				return EXIT_FAILURE;
			}
			break;
		case 'W':
			warn_f = 1;
			break;
		case 'h':
		default:
			print_usage(argv[0]);
			return EXIT_FAILURE;
			break;
		}
	if (geteuid()) {
		fatalp("You have to run this as ROOT\n");
		return EXIT_FAILURE;
	}

	cpf = conf_parsefile(conffile);
	if (cpf < 0 && strcmp(conffile, CONFFILE)) {
		fatalp("Cannot parse config file: %s\n", conffile);
		return EXIT_FAILURE;
	} else if (cpf > 0) {
		fatalp("Cannot parse line %d in config file\n", cpf);
		return EXIT_FAILURE;
	}

	if (set_my_ldp_id()) {
		fatalp("Cannot set LDP ID\n");
		return EXIT_FAILURE;
	}
	if (conf_ldp_id.s_addr != 0)
		strlcpy(my_ldp_id, inet_ntoa(conf_ldp_id), INET_ADDRSTRLEN);

	if (mplssockaddr.sa_len == 0) {
		fatalp("You need one mpls interface up and an IP "
		    "address set for it\n");
		return EXIT_FAILURE;
	}
	if (mpls_start_ldp() == -1)
		return EXIT_FAILURE;
	if (!strcmp(LDP_ID, "0.0.0.0")) {
		fatalp("Cannot set my LDP ID.\nAre you sure you've "
		    "got a non-loopback INET interface UP ?\n");
		return EXIT_FAILURE;
	}
	init_command_sockets();
	if ((command_socket = create_command_socket(command_port)) < 1) {
		fatalp("Cannot create command socket\n");
		return EXIT_FAILURE;
	}
	if (create_hello_sockets() != 0) {
		fatalp("Cannot create hello socket\n");
		return EXIT_FAILURE;
	}

	ls = create_listening_socket();

	if (ls < 0) {
		fatalp("Cannot create listening socket\n");
		return EXIT_FAILURE;
	}

	if (dontfork == 1)
		return the_big_loop();

	forkres = fork();
	if (forkres == 0) {
		syslog_f = 1;
		return the_big_loop();
	}
	if (forkres < 0)
		perror("fork");

	return EXIT_SUCCESS;
}
