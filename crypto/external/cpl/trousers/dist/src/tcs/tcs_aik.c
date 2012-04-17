
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "tcsps.h"
#include "tcslog.h"
#include "tcsd_wrap.h"
#include "tcsd.h"
#include "tcs_aik.h"

void
LoadBlob_SYMMETRIC_KEY(UINT64 *offset, BYTE *blob, TCPA_SYMMETRIC_KEY *key)
{
	LoadBlob_UINT32(offset, key->algId, blob);
	LoadBlob_UINT16(offset, key->encScheme, blob);
	LoadBlob_UINT16(offset, key->size, blob);

	if (key->size > 0) {
		LoadBlob(offset, key->size, blob, key->data);
	} else {
		key->data = NULL;
	}
}

TSS_RESULT
UnloadBlob_SYMMETRIC_KEY(UINT64 *offset, BYTE *blob, TCPA_SYMMETRIC_KEY *key)
{
	if (!key) {
		UINT16 size;

		UnloadBlob_UINT32(offset, NULL, blob);
		UnloadBlob_UINT16(offset, NULL, blob);
		UnloadBlob_UINT16(offset, &size, blob);

		if (size > 0)
			UnloadBlob(offset, size, blob, NULL);

		return TSS_SUCCESS;
	}

	UnloadBlob_UINT32(offset, &key->algId, blob);
	UnloadBlob_UINT16(offset, &key->encScheme, blob);
	UnloadBlob_UINT16(offset, &key->size, blob);

	if (key->size > 0) {
		key->data = (BYTE *)malloc(key->size);
		if (key->data == NULL) {
			LogError("malloc of %hu bytes failed.", key->size);
			key->size = 0;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(offset, key->size, blob, key->data);
	} else {
		key->data = NULL;
	}

	return TSS_SUCCESS;
}

void
get_credential(UINT32 type, UINT32 *size, BYTE **cred)
{
	int rc, fd;
	char *path = NULL;
	void *file = NULL;
	struct stat stat_buf;
	size_t file_size;

	switch (type) {
		case TSS_TCS_CREDENTIAL_PLATFORMCERT:
			path = tcsd_options.platform_cred;
			break;
		case TSS_TCS_CREDENTIAL_TPM_CC:
			path = tcsd_options.conformance_cred;
			break;
		case TSS_TCS_CREDENTIAL_EKCERT:
			path = tcsd_options.endorsement_cred;
			break;
		default:
			LogDebugFn("Bad credential type");
			break;
	}

	if (path == NULL)
		goto done;

	if ((fd = open(path, O_RDONLY)) < 0) {
		LogError("open(%s): %s", path, strerror(errno));
		goto done;
	}

	if ((rc = fstat(fd, &stat_buf)) == -1) {
		LogError("Error stating credential: %s: %s", path, strerror(errno));
		goto done;
	}

	file_size = (size_t)stat_buf.st_size;

	LogDebugFn("%s, (%zd bytes)", path, file_size);

	file = mmap(0, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (file == MAP_FAILED) {
		LogError("Error reading credential: %s: %s", path, strerror(errno));
		close(fd);
		goto done;
	}
	close(fd);

	if ((*cred = malloc(file_size)) == NULL) {
		LogError("malloc of %zd bytes failed.", file_size);
		munmap(file, file_size);
		goto done;
	}

	memcpy(*cred, file, file_size);
	*size = file_size;
	munmap(file, file_size);

	return;
done:
	*cred = NULL;
	*size = 0;
}
