
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2005, 2007, 2013
 *
 */

/* Defines which environment var is responsible for setting the port to
 * which the client will connect */
#define PORT_ENV_VAR "TSS_TCSD_PORT"

/* Defines which envionment var is responsible for setting the hostname to
 * which the client will connect */
#define HOSTNAME_ENV_VAR "TSS_TCSD_HOSTNAME"

#define TCP_PORT_STR_MAX_LEN 6

/* Prototypes for functions which retrieve tcsd hostname and port
 * information */
TSS_RESULT
get_tcsd_port(char port_str[TCP_PORT_STR_MAX_LEN]);

TSS_RESULT
get_tcsd_hostname(char **host_str, unsigned *len);
