/* dhcpd.h

   Definitions for dhcpd... */

/*
 * Copyright (c) 1995, 1996 The Internet Software Consortium.
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
 * by Ted Lemon <mellon@fugue.com> in cooperation with Vixie
 * Enterprises.  To learn more about the Internet Software Consortium,
 * see ``http://www.vix.com/isc''.  To learn more about Vixie
 * Enterprises, see ``http://www.vix.com''.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>

#include "cdefs.h"
#include "osdep.h"
#include "dhcp.h"
#include "tree.h"
#include "hash.h"
#include "inet.h"

/* A dhcp packet and the pointers to its option values. */
struct packet {
	struct dhcp_packet *raw;
	int packet_length;
	int packet_type;
	int options_valid;
	int client_port;
	struct iaddr client_addr;
	struct interface_info *interface;	/* Interface on which packet
						   was received. */
	struct hardware *haddr;		/* Physical link address
					   of local sender (maybe gateway). */
	struct shared_network *shared_network;
	struct {
		int len;
		unsigned char *data;
	} options [256];
};

struct hardware {
	u_int8_t htype;
	u_int8_t hlen;
	u_int8_t haddr [16];
};

/* A dhcp lease declaration structure. */
struct lease {
	struct lease *next;
	struct lease *prev;
	struct lease *n_uid, *n_hw;
	struct iaddr ip_addr;
	TIME starts, ends, timestamp;
	TIME offered_expiry;
	unsigned char *uid;
	int uid_len;
	char *hostname;
	struct host_decl *host;
	struct subnet *subnet;
	struct shared_network *shared_network;
	struct hardware hardware_addr;
	int state;
	int xid;
	int flags;
#       define STATIC_LEASE		1
#       define BOOTP_LEASE		2
#	define DYNAMIC_BOOTP_OK		4
#	define PERSISTENT_FLAGS		(DYNAMIC_BOOTP_OK)
#	define EPHEMERAL_FLAGS		(BOOTP_LEASE)
};

#define	ROOT_GROUP	0
#define HOST_DECL	1
#define SHARED_NET_DECL	2
#define SUBNET_DECL	3
#define CLASS_DECL	4
#define	GROUP_DECL	5

/* Group of declarations that share common parameters. */
struct group {
	struct group *next;

	struct subnet *subnet;
	struct shared_network *shared_network;

	TIME default_lease_time;
	TIME max_lease_time;
	TIME bootp_lease_cutoff;
	TIME bootp_lease_length;

	char *filename;
	char *server_name;	
	struct iaddr next_server;

	int boot_unknown_clients;
	int dynamic_bootp;
	int one_lease_per_client;
	int get_lease_hostnames;
	int use_host_decl_names;

	struct tree_cache *options [256];
};

/* A dhcp host declaration structure. */
struct host_decl {
	struct host_decl *n_ipaddr;
	char *name;
	struct hardware interface;
	struct tree_cache *fixed_addr;
	struct group *group;
};

struct shared_network {
	struct shared_network *next;
	char *name;
	struct subnet *subnets;
	struct interface_info *interface;
	struct lease *leases;
	struct lease *insertion_point;
	struct lease *last_lease;

	struct group *group;
};

struct subnet {
	struct subnet *next_subnet;
	struct subnet *next_sibling;
	struct shared_network *shared_network;
	struct interface_info *interface;
	struct iaddr interface_address;
	struct iaddr net;
	struct iaddr netmask;

	struct group *group;
};

struct class {
	char *name;

	struct group *group;
};

/* Information about each network interface. */

struct interface_info {
	struct interface_info *next;	/* Next interface in list... */
	struct shared_network *shared_network;
				/* Networks connected to this interface. */
	struct hardware hw_address;	/* Its physical address. */
	char name [IFNAMSIZ];		/* Its name... */
	int rfdesc;			/* Its read file descriptor. */
	int wfdesc;			/* Its write file descriptor, if
					   different. */
	unsigned char *rbuf;		/* Read buffer, if required. */
	size_t rbuf_max;		/* Size of read buffer. */
	size_t rbuf_offset;		/* Current offset into buffer. */
	size_t rbuf_len;		/* Length of data in buffer. */

	struct ifreq *tif;		/* Temp. pointer to ifreq struct. */
	u_int32_t flags;		/* Control flags... */
#define INTERFACE_REQUESTED 1
};

struct hardware_link {
	struct hardware_link *next;
	char name [IFNAMSIZ];
	struct hardware address;
};

/* Bitmask of dhcp option codes. */
typedef unsigned char option_mask [16];

/* DHCP Option mask manipulation macros... */
#define OPTION_ZERO(mask)	(memset (mask, 0, 16))
#define OPTION_SET(mask, bit)	(mask [bit >> 8] |= (1 << (bit & 7)))
#define OPTION_CLR(mask, bit)	(mask [bit >> 8] &= ~(1 << (bit & 7)))
#define OPTION_ISSET(mask, bit)	(mask [bit >> 8] & (1 << (bit & 7)))
#define OPTION_ISCLR(mask, bit)	(!OPTION_ISSET (mask, bit))

/* An option occupies its length plus two header bytes (code and
    length) for every 255 bytes that must be stored. */
#define OPTION_SPACE(x)		((x) + 2 * ((x) / 255 + 1))

/* Default path to dhcpd config file. */
#ifdef DEBUG
#undef _PATH_DHCPD_CONF
#define _PATH_DHCPD_CONF	"dhcpd.conf"
#undef _PATH_DHCPD_DB
#define _PATH_DHCPD_DB		"dhcpd.leases"
#else
#ifndef _PATH_DHCPD_CONF
#define _PATH_DHCPD_CONF	"/etc/dhcpd.conf"
#endif

#ifndef _PATH_DHCPD_DB
#define _PATH_DHCPD_DB		"/etc/dhcpd.leases"
#endif

#ifndef _PATH_DHCPD_PID
#define _PATH_DHCPD_PID		"/var/run/dhcpd.pid"
#endif
#endif

#ifndef DHCPD_LOG_FACILITY
#define DHCPD_LOG_FACILITY	LOG_DAEMON
#endif

#define MAX_TIME 0x7fffffff
#define MIN_TIME 0

/* External definitions... */

/* options.c */

void parse_options PROTO ((struct packet *));
void parse_option_buffer PROTO ((struct packet *, unsigned char *, int));
void cons_options PROTO ((struct packet *, struct packet *,
			  struct tree_cache **, int, int));
int store_options PROTO ((unsigned char *, int, struct tree_cache **,
			   unsigned char *, int, int, int, int));
char *pretty_print_option PROTO ((unsigned char, unsigned char *, int));

/* errwarn.c */
extern int warnings_occurred;
void error PROTO ((char *, ...));
int warn PROTO ((char *, ...));
int note PROTO ((char *, ...));
int debug PROTO ((char *, ...));
int parse_warn PROTO ((char *, ...));

/* dhcpd.c */
extern TIME cur_time;
extern struct group root_group;

extern struct iaddr server_identifier;
extern int server_identifier_matched;

extern u_int16_t server_port;
extern int log_priority;
extern int log_perror;

#ifdef USE_FALLBACK
extern struct interface_info fallback_interface;
#endif

extern char *path_dhcpd_conf;
extern char *path_dhcpd_db;
extern char *path_dhcpd_pid;

int main PROTO ((int, char **, char **));
void cleanup PROTO ((void));

/* conflex.c */
extern int lexline, lexchar;
extern char *token_line, *tlname;
extern char comments [4096];
extern int comment_index;
void new_parse PROTO ((char *));
int next_token PROTO ((char **, FILE *));
int peek_token PROTO ((char **, FILE *));

/* confpars.c */
int readconf PROTO ((void));
void read_leases PROTO ((void));
int parse_statement PROTO ((FILE *,
			    struct group *, int, struct host_decl *, int));
void skip_to_semi PROTO ((FILE *));
int parse_boolean PROTO ((FILE *));
int parse_semi PROTO ((FILE *));
int parse_lbrace PROTO ((FILE *));
void parse_host_declaration PROTO ((FILE *, struct group *));
char *parse_host_name PROTO ((FILE *));
void parse_class_declaration PROTO ((FILE *, struct group *, int));
void parse_lease_time PROTO ((FILE *, TIME *));
void parse_shared_net_declaration PROTO ((FILE *, struct group *));
void parse_subnet_declaration PROTO ((FILE *, struct shared_network *));
void parse_group_declaration PROTO ((FILE *, struct group *));
void parse_hardware_param PROTO ((FILE *, struct hardware *));
char *parse_string PROTO ((FILE *));
struct tree *parse_ip_addr_or_hostname PROTO ((FILE *, int));
struct tree_cache *parse_fixed_addr_param PROTO ((FILE *));
void parse_option_param PROTO ((FILE *, struct group *));
TIME parse_timestamp PROTO ((FILE *));
struct lease *parse_lease_declaration PROTO ((FILE *));
void parse_address_range PROTO ((FILE *, struct subnet *));
TIME parse_date PROTO ((FILE *));
unsigned char *parse_numeric_aggregate PROTO ((FILE *,
					       unsigned char *, int *,
					       int, int, int));
void convert_num PROTO ((unsigned char *, char *, int, int));

/* tree.c */
pair cons PROTO ((caddr_t, pair));
struct tree_cache *tree_cache PROTO ((struct tree *));
struct tree *tree_host_lookup PROTO ((char *));
struct dns_host_entry *enter_dns_host PROTO ((char *));
struct tree *tree_const PROTO ((unsigned char *, int));
struct tree *tree_concat PROTO ((struct tree *, struct tree *));
struct tree *tree_limit PROTO ((struct tree *, int));
int tree_evaluate PROTO ((struct tree_cache *));

/* dhcp.c */
void dhcp PROTO ((struct packet *));
void dhcpdiscover PROTO ((struct packet *));
void dhcprequest PROTO ((struct packet *));
void dhcprelease PROTO ((struct packet *));
void dhcpdecline PROTO ((struct packet *));
void dhcpinform PROTO ((struct packet *));
void nak_lease PROTO ((struct packet *, struct iaddr *cip));
void ack_lease PROTO ((struct packet *, struct lease *, unsigned char, TIME));
struct lease *find_lease PROTO ((struct packet *, struct shared_network *));
struct lease *mockup_lease PROTO ((struct packet *,
				   struct shared_network *,
				   struct host_decl *));

/* bootp.c */
void bootp PROTO ((struct packet *));

/* memory.c */
void enter_host PROTO ((struct host_decl *));
struct host_decl *find_hosts_by_haddr PROTO ((int, unsigned char *, int));
struct host_decl *find_hosts_by_uid PROTO ((unsigned char *, int));
struct subnet *find_host_for_network PROTO ((struct host_decl **,
					     struct iaddr *,
					     struct shared_network *));
void new_address_range PROTO ((struct iaddr, struct iaddr,
			       struct subnet *, int));
extern struct subnet *find_grouped_subnet PROTO ((struct shared_network *,
						  struct iaddr));
extern struct subnet *find_subnet PROTO ((struct iaddr));
void enter_shared_network PROTO ((struct shared_network *));
void enter_subnet PROTO ((struct subnet *));
void enter_lease PROTO ((struct lease *));
int supersede_lease PROTO ((struct lease *, struct lease *, int));
void release_lease PROTO ((struct lease *));
void abandon_lease PROTO ((struct lease *));
struct lease *find_lease_by_uid PROTO ((unsigned char *, int));
struct lease *find_lease_by_hw_addr PROTO ((unsigned char *, int));
struct lease *find_lease_by_ip_addr PROTO ((struct iaddr));
void uid_hash_add PROTO ((struct lease *));
void uid_hash_delete PROTO ((struct lease *));
void hw_hash_add PROTO ((struct lease *));
void hw_hash_delete PROTO ((struct lease *));
struct class *add_class PROTO ((int, char *));
struct class *find_class PROTO ((int, char *, int));
struct group *clone_group PROTO ((struct group *, char *));
void write_leases PROTO ((void));
void dump_subnets PROTO ((void));

/* alloc.c */
VOIDPTR dmalloc PROTO ((int, char *));
void dfree PROTO ((VOIDPTR, char *));
struct packet *new_packet PROTO ((char *));
struct dhcp_packet *new_dhcp_packet PROTO ((char *));
struct tree *new_tree PROTO ((char *));
struct tree_cache *new_tree_cache PROTO ((char *));
struct hash_table *new_hash_table PROTO ((int, char *));
struct hash_bucket *new_hash_bucket PROTO ((char *));
struct lease *new_lease PROTO ((char *));
struct lease *new_leases PROTO ((int, char *));
struct subnet *new_subnet PROTO ((char *));
struct class *new_class PROTO ((char *));
struct shared_network *new_shared_network PROTO ((char *));
struct group *new_group PROTO ((char *));
void free_group PROTO ((struct group *, char *));
void free_shared_network PROTO ((struct shared_network *, char *));
void free_class PROTO ((struct class *, char *));
void free_subnet PROTO ((struct subnet *, char *));
void free_lease PROTO ((struct lease *, char *));
void free_hash_bucket PROTO ((struct hash_bucket *, char *));
void free_hash_table PROTO ((struct hash_table *, char *));
void free_tree_cache PROTO ((struct tree_cache *, char *));
void free_packet PROTO ((struct packet *, char *));
void free_dhcp_packet PROTO ((struct dhcp_packet *, char *));
void free_tree PROTO ((struct tree *, char *));

/* print.c */
char *print_hw_addr PROTO ((int, int, unsigned char *));
void print_lease PROTO ((struct lease *));
void dump_raw PROTO ((unsigned char *, int));
void dump_packet PROTO ((struct packet *));
void hash_dump PROTO ((struct hash_table *));

/* socket.c */
#if defined (USE_SOCKET_SEND) || defined (USE_SOCKET_RECEIVE) \
	|| defined (USE_SOCKET_FALLBACK)
int if_register_socket PROTO ((struct interface_info *, struct ifreq *));
#endif

#ifdef USE_SOCKET_FALLBACK
void if_register_fallback PROTO ((struct interface_info *, struct ifreq *));
size_t send_fallback PROTO ((struct interface_info *,
			   struct packet *, struct dhcp_packet *, size_t, 
			   struct in_addr,
			   struct sockaddr_in *, struct hardware *));
size_t fallback_discard PROTO ((struct interface_info *));
#endif

#ifdef USE_SOCKET_SEND
void if_register_send PROTO ((struct interface_info *, struct ifreq *));
size_t send_packet PROTO ((struct interface_info *,
			   struct packet *, struct dhcp_packet *, size_t, 
			   struct in_addr,
			   struct sockaddr_in *, struct hardware *));
#endif
#ifdef USE_SOCKET_RECEIVE
void if_register_receive PROTO ((struct interface_info *, struct ifreq *));
size_t receive_packet PROTO ((struct interface_info *,
			   unsigned char *, size_t,
			   struct sockaddr_in *, struct hardware *));
#endif

/* bpf.c */
#if defined (USE_BPF_SEND) || defined (USE_BPF_RECEIVE)
int if_register_bpf PROTO ( (struct interface_info *, struct ifreq *));
#endif
#ifdef USE_BPF_SEND
void if_register_send PROTO ((struct interface_info *, struct ifreq *));
size_t send_packet PROTO ((struct interface_info *,
			   struct packet *, struct dhcp_packet *, size_t,
			   struct in_addr,
			   struct sockaddr_in *, struct hardware *));
#endif
#ifdef USE_BPF_RECEIVE
void if_register_receive PROTO ((struct interface_info *, struct ifreq *));
size_t receive_packet PROTO ((struct interface_info *,
			   unsigned char *, size_t,
			   struct sockaddr_in *, struct hardware *));
#endif

/* nit.c */
#ifdef USE_NIT_SEND
void if_register_send PROTO ((struct interface_info *, struct ifreq *));
size_t send_packet PROTO ((struct interface_info *,
			   struct packet *, struct dhcp_packet *, size_t,
			   struct in_addr,
			   struct sockaddr_in *, struct hardware *));
#endif
#ifdef USE_NIT_RECEIVE
void if_register_receive PROTO ((struct interface_info *, struct ifreq *));
size_t receive_packet PROTO ((struct interface_info *,
			   unsigned char *, size_t,
			   struct sockaddr_in *, struct hardware *));
#endif

/* raw.c */
#ifdef USE_RAW_SEND
void if_register_send PROTO ((struct interface_info *, struct ifreq *));
size_t send_packet PROTO ((struct interface_info *,
			   struct packet *, struct dhcp_packet *, size_t,
			   struct in_addr,
			   struct sockaddr_in *, struct hardware *));
#endif

/* dispatch.c */
extern struct interface_info *interfaces;
void discover_interfaces PROTO ((int));
void dispatch PROTO ((void));
void do_packet PROTO ((struct interface_info *,
		       unsigned char *, int,
		       unsigned short, struct iaddr, struct hardware *));
int locate_network PROTO ((struct packet *));

/* hash.c */
struct hash_table *new_hash PROTO ((void));
void add_hash PROTO ((struct hash_table *, char *, int, unsigned char *));
void delete_hash_entry PROTO ((struct hash_table *, char *, int));
unsigned char *hash_lookup PROTO ((struct hash_table *, char *, int));

/* tables.c */
extern struct option dhcp_options [256];
extern unsigned char dhcp_option_default_priority_list [];
extern int sizeof_dhcp_option_default_priority_list;
extern char *hardware_types [256];
extern struct hash_table universe_hash;
extern struct universe dhcp_universe;
void initialize_universes PROTO ((void));

/* convert.c */
u_int32_t getULong PROTO ((unsigned char *));
int32_t getLong PROTO ((unsigned char *));
u_int16_t getUShort PROTO ((unsigned char *));
int16_t getShort PROTO ((unsigned char *));
void putULong PROTO ((unsigned char *, u_int32_t));
void putLong PROTO ((unsigned char *, int32_t));
void putUShort PROTO ((unsigned char *, u_int16_t));
void putShort PROTO ((unsigned char *, int16_t));

/* inet.c */
struct iaddr subnet_number PROTO ((struct iaddr, struct iaddr));
struct iaddr ip_addr PROTO ((struct iaddr, struct iaddr, u_int32_t));
u_int32_t host_addr PROTO ((struct iaddr, struct iaddr));
int addr_eq PROTO ((struct iaddr, struct iaddr));
char *piaddr PROTO ((struct iaddr));

/* dhclient.c */
void dhcpoffer PROTO ((struct packet *));
void dhcpack PROTO ((struct packet *));
void dhcpnak PROTO ((struct packet *));
void send_discover PROTO ((struct interface_info *));
void send_request PROTO ((struct packet *));

/* db.c */
int write_lease PROTO ((struct lease *));
int commit_leases PROTO ((void));
void db_startup PROTO ((void));
void new_lease_file PROTO ((void));

/* packet.c */
void assemble_hw_header PROTO ((struct interface_info *, unsigned char *,
				int *, struct hardware *));
void assemble_udp_ip_header PROTO ((struct interface_info *, unsigned char *,
				    int *, u_int32_t, u_int32_t, u_int16_t,
				    unsigned char *, int));
size_t decode_hw_header PROTO ((struct interface_info *, unsigned char *,
				int, struct hardware *));
size_t decode_udp_ip_header PROTO ((struct interface_info *, unsigned char *,
				    int, struct sockaddr_in *,
				    unsigned char *, int));

/* dhxpxlt.c */
void convert_statement PROTO ((FILE *));
void convert_host_statement PROTO ((FILE *, jrefproto));
void convert_host_name PROTO ((FILE *, jrefproto));
void convert_class_statement PROTO ((FILE *, jrefproto, int));
void convert_class_decl PROTO ((FILE *, jrefproto));
void convert_lease_time PROTO ((FILE *, jrefproto, char *));
void convert_shared_net_statement PROTO ((FILE *, jrefproto));
void convert_subnet_statement PROTO ((FILE *, jrefproto));
void convert_subnet_decl PROTO ((FILE *, jrefproto));
void convert_host_decl PROTO ((FILE *, jrefproto));
void convert_hardware_decl PROTO ((FILE *, jrefproto));
void convert_hardware_addr PROTO ((FILE *, jrefproto));
void convert_filename_decl PROTO ((FILE *, jrefproto));
void convert_servername_decl PROTO ((FILE *, jrefproto));
void convert_ip_addr_or_hostname PROTO ((FILE *, jrefproto, int));
void convert_fixed_addr_decl PROTO ((FILE *, jrefproto));
void convert_option_decl PROTO ((FILE *, jrefproto));
void convert_timestamp PROTO ((FILE *, jrefproto));
void convert_lease_statement PROTO ((FILE *, jrefproto));
void convert_address_range PROTO ((FILE *, jrefproto));
void convert_date PROTO ((FILE *, jrefproto, char *));
void convert_numeric_aggregate PROTO ((FILE *, jrefproto, int, int, int, int));
void indent PROTO ((int));
