
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2005, 2007, 2013
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#ifndef __APPLE__
#include <limits.h>
#endif
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "tsplog.h"
#include "spi_utils.h"
#include "tsp_tcsi_param.h"

#define RV_OK 0
#define RV_NO_VALUE -1
#define RV_NO_MEM -2
#define RV_WRONG_VALUE -3
#define RV_UNKNOWN_ERR -4

int
get_port_from_env(int *port)
{
	char *env_port;
	char *raw_port_str;

	env_port = getenv(PORT_ENV_VAR);
	if (env_port == NULL)
		return RV_NO_VALUE;

	raw_port_str = strdup(env_port);
	if (raw_port_str == NULL)
		return RV_NO_MEM;

	LogDebug("Found data in %s environment var: %s", PORT_ENV_VAR, raw_port_str);

	*port = atoi(raw_port_str);
	free(raw_port_str);
	if (*port < 0 || *port > 65535) {
		LogError("Environment var %s contains invalid port value!", PORT_ENV_VAR);
		return RV_WRONG_VALUE;
	}

	return RV_OK;
}

TSS_RESULT
convert_port_to_str(int port, char port_str[TCP_PORT_STR_MAX_LEN])
{
	if (snprintf(port_str, TCP_PORT_STR_MAX_LEN, "%d", port) < 0)
		return TSPERR(TSS_E_INTERNAL_ERROR);

	return TSS_SUCCESS;
}

TSS_RESULT
get_tcsd_port(char port_str[TCP_PORT_STR_MAX_LEN])
{
	int rv, port = 0;

	// Retrieves port from env var first
	rv = get_port_from_env(&port);
	switch(rv) {
		case RV_OK:
			return convert_port_to_str(port, port_str);
		case RV_WRONG_VALUE:
			return TSPERR(TSS_E_BAD_PARAMETER);
		case RV_NO_MEM:
			return TSPERR(TSS_E_OUTOFMEMORY);
		case RV_NO_VALUE:
			break;
	}

	// TODO: Future work retrieves port from config file.

	// Last case, retrieve default port used by server.
	return convert_port_to_str(TCSD_DEFAULT_PORT, port_str);
}

/**
 *  Allocates a string with up to HOST_NAME_MAX chars which contains
 *  the hostname extracted from the env var
 */
int
get_hostname_from_env(char **host_str, unsigned *len)
{
	char *env_host, *tmp_str = NULL;
	size_t env_len;

	// Tries to retrieve from env var first.
	env_host = getenv(HOSTNAME_ENV_VAR);
	if (env_host == NULL) {
		*host_str = NULL;
		*len = 0;
		LogDebug("Got no value inside environment var %s.", HOSTNAME_ENV_VAR);
		return RV_NO_VALUE;
	}

	tmp_str = strdup(env_host);
	if (tmp_str == NULL)
		return RV_NO_MEM;

	LogDebug("Environment var %s got value: %s", HOSTNAME_ENV_VAR, tmp_str);
	env_len = strlen(tmp_str);
	if (env_len > HOST_NAME_MAX) {
		*len = HOST_NAME_MAX + 1;
	} else {
		*len = env_len + 1;
	}

	*host_str = (char *)malloc(*len);
	if (*host_str == NULL) {
		LogError("Not enough memory when allocating string to retrieve host	name from environment var");
		free(tmp_str);
		return RV_NO_MEM;
	}

	strncpy(*host_str, tmp_str, *len);
	free(tmp_str);
	return RV_OK;
}

TSS_RESULT
get_tcsd_hostname(char **host_str, unsigned *len)
{
	int rv;

	// Retrieve from environment var
	rv = get_hostname_from_env(host_str, len);
	switch(rv) {
	case RV_OK:
		LogDebug("Hostname %s will be used", *host_str);
		return TSS_SUCCESS;
	case RV_NO_MEM:
		return TSPERR(TSS_E_OUTOFMEMORY);
	case RV_NO_VALUE: // Tente obter de outra maneira
		break;
	default:
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	// TODO: Future work - Retrieve from config file

	// Use localhost in last case.
	*host_str = strdup(TSS_LOCALHOST_STRING);
	if (*host_str == NULL)
		return TSPERR(TSS_E_OUTOFMEMORY);

	*len = sizeof(TSS_LOCALHOST_STRING);

	LogDebug("Hostname %s will be used", *host_str);
	return TSS_SUCCESS;
}

