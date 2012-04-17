
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "spi_utils.h"
#include "tspps.h"
#include "tsplog.h"

TSS_RESULT
read_data(int fd, void *data, UINT32 size)
{
	int rc;

	rc = read(fd, data, size);
	if (rc == -1) {
		LogError("read of %d bytes: %s", size, strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	} else if ((unsigned)rc != size) {
		LogError("read of %d bytes (only %d read)", size, rc);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return TSS_SUCCESS;
}

TSS_RESULT
write_data(int fd, void *data, UINT32 size)
{
	int rc;

	rc = write(fd, data, size);
	if (rc == -1) {
		LogError("write of %d bytes: %s", size, strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	} else if ((unsigned)rc != size) {
		LogError("write of %d bytes (only %d written)", size, rc);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return TSS_SUCCESS;
}
