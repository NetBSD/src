/* dhcpd.c

   DHCP Server Daemon. */

/*
 * Copyright (c) 1996-2000 Internet Software Consortium.
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

#ifndef lint
static char ocopyright[] =
"$Id: dhcpd.c,v 1.15 2000/04/22 08:18:17 mellon Exp $ Copyright 1995-2000 The Internet Software Consortium.";
#endif

  static char copyright[] =
"Copyright 1995, 1996, 1997, 1998, 1999 The Internet Software Consortium.";
static char arr [] = "All rights reserved.";
static char message [] = "Internet Software Consortium DHCP Server";
static char contrib [] = "\nPlease contribute if you find this software useful.";
static char url [] = "For info, please visit http://www.isc.org/dhcp-contrib.html\n";

#include "dhcpd.h"
#include "version.h"
#include <omapip/omapip_p.h>

static void usage PROTO ((void));

TIME cur_time;
struct group root_group;
struct binding_scope global_scope;

struct iaddr server_identifier;
int server_identifier_matched;

u_int16_t local_port;
u_int16_t remote_port;

struct in_addr limited_broadcast;

/* This is the standard name service updater that is executed whenever a
   lease is committed.   Right now it's not following the DHCP-DNS draft
   at all, but as soon as I fix the resolver it should try to. */

#if defined (NSUPDATE)
char std_nsupdate [] = "						    \n\
on commit {								    \n\
  if (not defined (ddns-fwd-name)) {					    \n\
    set ddns-fwd-name = concat (pick (config-option server.ddns-hostname,   \n\
				      option host-name), \".\",		    \n\
			        pick (config-option server.ddns-domainname, \n\
				      config-option domain-name));	    \n\
    if defined (ddns-fwd-name) {					    \n\
      switch (ns-update (not exists (IN, A, ddns-fwd-name, null),	    \n\
		         add (IN, A, ddns-fwd-name, leased-address,	    \n\
			      lease-time / 2))) {			    \n\
       default:								    \n\
        unset ddns-fwd-name;						    \n\
        break;								    \n\
									    \n\
       case NOERROR:							    \n\
        set ddns-rev-name =						    \n\
		concat (binary-to-ascii (10, 8, \".\",			    \n\
					 reverse (1,			    \n\
						  leased-address)), \".\",  \n\
			pick (config-option server.ddns-rev-domainname,	    \n\
			      \"in-addr.arpa.\"));			    \n\
        switch (ns-update (delete (IN, PTR, ddns-rev-name, null),	    \n\
			   add (IN, PTR, ddns-rev-name, ddns-fwd-name,	    \n\
				lease-time / 2)))			    \n\
	{								    \n\
         default:							    \n\
	  unset ddns-rev-name;						    \n\
	  on release or expiry {					    \n\
	    switch (ns-update (delete (IN, A, ddns-fwd-name,		    \n\
				       leased-address))) {		    \n\
	      case NOERROR:						    \n\
	        unset ddns-fwd-name;					    \n\
		break;							    \n\
	    }								    \n\
	    on release or expiry;					    \n\
          }								    \n\
	  break;							    \n\
									    \n\
         case NOERROR:							    \n\
	  on release or expiry {					    \n\
	    switch (ns-update (delete (IN, PTR, ddns-rev-name, null))) {    \n\
	      case NOERROR:						    \n\
	        unset ddns-rev-name;					    \n\
		break;							    \n\
	    }								    \n\
	    switch (ns-update (delete (IN, A, ddns-fwd-name,		    \n\
			               leased-address))) {		    \n\
	      case NOERROR:						    \n\
	        unset ddns-fwd-name;					    \n\
		break;							    \n\
	    }								    \n\
	    on release or expiry;					    \n\
	  }								    \n\
        }								    \n\
      }									    \n\
    }									    \n\
  }									    \n\
}";
#endif /* NSUPDATE */

const char *path_dhcpd_conf = _PATH_DHCPD_CONF;
const char *path_dhcpd_db = _PATH_DHCPD_DB;
const char *path_dhcpd_pid = _PATH_DHCPD_PID;

int dhcp_max_agent_option_packet_length = DHCP_MTU_MAX;

int main (argc, argv, envp)
	int argc;
	char **argv, **envp;
{
	int i, status;
	struct servent *ent;
	char *s;
	int cftest = 0;
	int lftest = 0;
#ifndef DEBUG
	int pidfilewritten = 0;
	int pid;
	char pbuf [20];
	int daemon = 1;
#endif
	int quiet = 0;
	char *server = (char *)0;
	isc_result_t result;
	omapi_object_t *listener;
	unsigned seed;
	struct interface_info *ip;
	struct data_string fname;
	struct option_cache *oc;
	struct option_state *options = (struct option_state *)0;
	struct parse *parse;
	int lose;

	/* Initially, log errors to stderr as well as to syslogd. */
#ifdef SYSLOG_4_2
	openlog ("dhcpd", LOG_NDELAY);
	log_priority = DHCPD_LOG_FACILITY;
#else
	openlog ("dhcpd", LOG_NDELAY, DHCPD_LOG_FACILITY);
#endif

	for (i = 1; i < argc; i++) {
		if (!strcmp (argv [i], "-p")) {
			if (++i == argc)
				usage ();
			for (s = argv [i]; *s; s++)
				if (!isdigit (*s))
					log_fatal ("%s: not a valid UDP port",
					       argv [i]);
			status = atoi (argv [i]);
			if (status < 1 || status > 65535)
				log_fatal ("%s: not a valid UDP port",
				       argv [i]);
			local_port = htons (status);
			log_debug ("binding to user-specified port %d",
			       ntohs (local_port));
		} else if (!strcmp (argv [i], "-f")) {
#ifndef DEBUG
			daemon = 0;
#endif
		} else if (!strcmp (argv [i], "-d")) {
#ifndef DEBUG
			daemon = 0;
#endif
			log_perror = -1;
		} else if (!strcmp (argv [i], "-s")) {
			if (++i == argc)
				usage ();
			server = argv [i];
		} else if (!strcmp (argv [i], "-cf")) {
			if (++i == argc)
				usage ();
			path_dhcpd_conf = argv [i];
		} else if (!strcmp (argv [i], "-lf")) {
			if (++i == argc)
				usage ();
			path_dhcpd_db = argv [i];
		} else if (!strcmp (argv [i], "-pf")) {
			if (++i == argc)
				usage ();
			path_dhcpd_pid = argv [i];
                } else if (!strcmp (argv [i], "-t")) {
			/* test configurations only */
#ifndef DEBUG
			daemon = 0;
#endif
			cftest = 1;
			log_perror = -1;
                } else if (!strcmp (argv [i], "-T")) {
			/* test configurations and lease file only */
#ifndef DEBUG
			daemon = 0;
#endif
			cftest = 1;
			lftest = 1;
			log_perror = -1;
		} else if (!strcmp (argv [i], "-q")) {
			quiet = 1;
			quiet_interface_discovery = 1;
		} else if (argv [i][0] == '-') {
			usage ();
		} else {
			struct interface_info *tmp =
				((struct interface_info *)
				 dmalloc (sizeof *tmp, MDL));
			if (!tmp)
				log_fatal ("Insufficient memory to %s %s",
				       "record interface", argv [i]);
			memset (tmp, 0, sizeof *tmp);
			strcpy (tmp -> name, argv [i]);
			tmp -> next = interfaces;
			tmp -> flags = INTERFACE_REQUESTED;
			interfaces = tmp;
		}
	}

	if (!quiet) {
		log_info ("%s %s", message, DHCP_VERSION);
		log_info (copyright);
		log_info (arr);
		log_info (contrib);
		log_info (url);
	} else {
		quiet = 0;
		log_perror = 0;
	}

	/* Default to the DHCP/BOOTP port. */
	if (!local_port)
	{
		ent = getservbyname ("dhcp", "udp");
		if (!ent)
			local_port = htons (67);
		else
			local_port = ent -> s_port;
#ifndef __CYGWIN32__ /* XXX */
		endservent ();
#endif
	}
  
	remote_port = htons (ntohs (local_port) + 1);

	if (server) {
		if (!inet_aton (server, &limited_broadcast)) {
			struct hostent *he;
			he = gethostbyname (server);
			if (he) {
				memcpy (&limited_broadcast,
					he -> h_addr_list [0],
					sizeof limited_broadcast);
			} else
				limited_broadcast.s_addr = INADDR_BROADCAST;
		}
	} else {
		limited_broadcast.s_addr = INADDR_BROADCAST;
	}

	/* Get the current time... */
	GET_TIME (&cur_time);

	/* Set up the client classification system. */
	classification_setup ();

	/* Initialize the omapi system. */
	result = omapi_init ();
	if (result != ISC_R_SUCCESS)
		log_fatal ("Can't initialize OMAPI: %s\n",
			   isc_result_totext (result));

	/* Set up the OMAPI wrappers for various server database internal
	   objects. */
	dhcp_db_objects_setup ();

	/* Set up the initial dhcp option universe. */
	initialize_common_option_spaces ();
	initialize_server_option_spaces ();

	root_group.authoritative = 0;

#if defined (NSUPDATE)
	/* Set up the standard name service updater routine. */
	parse = (struct parse *)0;
	status = new_parse (&parse, -1,
			    std_nsupdate, (sizeof std_nsupdate) - 1,
			    "standard name service update routine");
	if (status != ISC_R_SUCCESS)
		log_fatal ("can't parse standard name service updater!");

	if (!(parse_executable_statements
	      (&root_group.statements, parse, &lose, context_any))) {
		end_parse (&parse);
		log_fatal ("can't parse standard name service updater!");
	}
	end_parse (&parse);
#endif

	/* Read the dhcpd.conf file... */
	if (readconf () != ISC_R_SUCCESS)
		log_fatal ("Configuration file errors encountered -- exiting");

        /* test option should cause an early exit */
 	if (cftest && !lftest) 
 		exit(0);

	/* Now try to get the lease file name. */
	option_state_allocate (&options, MDL);

	execute_statements_in_scope ((struct packet *)0,
				     (struct lease *)0,
				     (struct option_state *)0,
				     options, &global_scope,
				     &root_group,
				     (struct group *)0);
	memset (&fname, 0, sizeof fname);
	oc = lookup_option (&server_universe, options, SV_LEASE_FILE_NAME);
	if (oc &&
	    evaluate_option_cache (&fname, (struct packet *)0,
				   (struct lease *)0, options,
				   (struct option_state *)0,
				   &global_scope, oc, MDL)) {
		s = dmalloc (fname.len + 1, MDL);
		if (!s)
			log_fatal ("no memory for lease db filename.");
		memcpy (s, fname.data, fname.len);
		s [fname.len] = 0;
		data_string_forget (&fname, MDL);
		path_dhcpd_db = s;
	}
	
	oc = lookup_option (&server_universe, options, SV_PID_FILE_NAME);
	if (oc &&
	    evaluate_option_cache (&fname, (struct packet *)0,
				   (struct lease *)0, options,
				   (struct option_state *)0,
				   &global_scope, oc, MDL)) {
		s = dmalloc (fname.len + 1, MDL);
		if (!s)
			log_fatal ("no memory for lease db filename.");
		memcpy (s, fname.data, fname.len);
		s [fname.len] = 0;
		data_string_forget (&fname, MDL);
		path_dhcpd_pid = s;
	}

	/* Don't need the options anymore. */
	option_state_dereference (&options, MDL);
	
	/* Start up the database... */
	db_startup (lftest);

	if (lftest)
		exit (0);

	/* Discover all the network interfaces and initialize them. */
	discover_interfaces (DISCOVER_SERVER);

	/* Make up a seed for the random number generator from current
	   time plus the sum of the last four bytes of each
	   interface's hardware address interpreted as an integer.
	   Not much entropy, but we're booting, so we're not likely to
	   find anything better. */
	seed = 0;
	for (ip = interfaces; ip; ip = ip -> next) {
		int junk;
		memcpy (&junk,
			&ip -> hw_address.hbuf [ip -> hw_address.hlen -
					       sizeof seed], sizeof seed);
		seed += junk;
	}
	srandom (seed + cur_time);

	/* Initialize icmp support... */
	icmp_startup (1, lease_pinged);

	/* Start up a listener for the object management API protocol. */
	listener = (omapi_object_t *)0;
	result = omapi_generic_new (&listener, MDL);
	if (result != ISC_R_SUCCESS)
		log_fatal ("Can't allocate new generic object: %s\n",
			   isc_result_totext (result));
	result = omapi_protocol_listen (listener,
					OMAPI_PROTOCOL_PORT, 1);
	if (result != ISC_R_SUCCESS)
		log_fatal ("Can't start OMAPI protocol: %s",
			   isc_result_totext (result));

#ifndef DEBUG
	if (daemon) {
		/* First part of becoming a daemon... */
		if ((pid = fork ()) < 0)
			log_fatal ("Can't fork daemon: %m");
		else if (pid)
			exit (0);
	}

	/* Read previous pid file. */
	if ((i = open (path_dhcpd_pid, O_RDONLY)) >= 0) {
		status = read (i, pbuf, (sizeof pbuf) - 1);
		close (i);
		pbuf [status] = 0;
		pid = atoi (pbuf);

		/* If the previous server process is not still running,
		   write a new pid file immediately. */
		if (pid && (pid == getpid() || kill (pid, 0) < 0)) {
			unlink (path_dhcpd_pid);
			if ((i = open (path_dhcpd_pid,
				       O_WRONLY | O_CREAT, 0640)) >= 0) {
				sprintf (pbuf, "%d\n", (int)getpid ());
				write (i, pbuf, strlen (pbuf));
				close (i);
				pidfilewritten = 1;
			}
		} else
			log_fatal ("There's already a DHCP server running.\n");
	}

	/* If we were requested to log to stdout on the command line,
	   keep doing so; otherwise, stop. */
	if (log_perror == -1)
		log_perror = 1;
	else
		log_perror = 0;

	if (daemon) {
		/* Become session leader and get pid... */
		close (0);
		close (1);
		close (2);
		pid = setsid ();
	}

	/* If we didn't write the pid file earlier because we found a
	   process running the logged pid, but we made it to here,
	   meaning nothing is listening on the bootp port, then write
	   the pid file out - what's in it now is bogus anyway. */
	if (!pidfilewritten) {
		unlink (path_dhcpd_pid);
		if ((i = open (path_dhcpd_pid,
			       O_WRONLY | O_CREAT, 0640)) >= 0) {
			sprintf (pbuf, "%d\n", (int)getpid ());
			write (i, pbuf, strlen (pbuf));
			close (i);
			pidfilewritten = 1;
		}
	}
#endif /* !DEBUG */

	/* Set up the bootp packet handler... */
	bootp_packet_handler = do_packet;

#if defined (DEBUG_MEMORY_LEAKAGE) || defined (DEBUG_MALLOC_POOL)
	dmalloc_cutoff_generation = dmalloc_generation;
	dmalloc_longterm = dmalloc_outstanding;
	dmalloc_outstanding = 0;
#endif

	/* Receive packets and dispatch them... */
	dispatch ();

	/* Not reached */
	return 0;
}

/* Print usage message. */

static void usage ()
{
	log_info (message);
	log_info (copyright);
	log_info (arr);

	log_fatal ("Usage: dhcpd [-p <UDP port #>] [-d] [-f]%s%s",
		   "\n             [-cf config-file] [-lf lease-file]",
		   "\n             [-t] [-T] [-s server] [if0 [...ifN]]");
}

void lease_pinged (from, packet, length)
	struct iaddr from;
	u_int8_t *packet;
	int length;
{
	struct lease *lp;

	/* Don't try to look up a pinged lease if we aren't trying to
	   ping one - otherwise somebody could easily make us churn by
	   just forging repeated ICMP EchoReply packets for us to look
	   up. */
	if (!outstanding_pings)
		return;

	lp = find_lease_by_ip_addr (from);

	if (!lp) {
		log_info ("unexpected ICMP Echo Reply from %s", piaddr (from));
		return;
	}

	if (!lp -> state) {
		log_error ("ICMP Echo Reply for %s late or spurious.\n",
		      piaddr (from));
		return;
	}

	if (lp -> ends > cur_time) {
		log_error ("ICMP Echo reply while lease %s valid\n",
			   piaddr (from));
	}

	/* At this point it looks like we pinged a lease and got a
	   response, which shouldn't have happened. */
	data_string_forget (&lp -> state -> parameter_request_list, MDL);
	free_lease_state (lp -> state, MDL);
	lp -> state = (struct lease_state *)0;

	abandon_lease (lp, "pinged before offer");
	cancel_timeout (lease_ping_timeout, lp);
	--outstanding_pings;
}

void lease_ping_timeout (vlp)
	void *vlp;
{
	struct lease *lp = vlp;

#if defined (DEBUG_MEMORY_LEAKAGE)
	unsigned long previous_outstanding = dmalloc_outstanding;
#endif

	--outstanding_pings;
	dhcp_reply (lp);

#if defined (DEBUG_MEMORY_LEAKAGE)
	log_info ("generation %ld: %ld new, %ld outstanding, %ld long-term",
		  dmalloc_generation,
		  dmalloc_outstanding - previous_outstanding,
		  dmalloc_outstanding, dmalloc_longterm);
#endif
#if defined (DEBUG_MEMORY_LEAKAGE) || defined (DEBUG_MALLOC_POOL)
	dmalloc_dump_outstanding ();
#endif
}
