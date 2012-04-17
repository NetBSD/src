
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
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "tcs_tsp.h"
#include "tspps.h"


void
Trspi_LoadBlob_DAA_PK(UINT64 *offset, BYTE *blob, TSS_DAA_PK *pk)
{
	UINT32 i;

	Trspi_LoadBlob_TSS_VERSION(offset, blob, pk->versionInfo);

	Trspi_LoadBlob_UINT32(offset, pk->modulusLength, blob);
	Trspi_LoadBlob(offset, pk->modulusLength, blob, pk->modulus);

	Trspi_LoadBlob_UINT32(offset, pk->capitalSLength, blob);
	Trspi_LoadBlob(offset, pk->capitalSLength, blob, pk->capitalS);

	Trspi_LoadBlob_UINT32(offset, pk->capitalZLength, blob);
	Trspi_LoadBlob(offset, pk->capitalZLength, blob, pk->capitalZ);

	Trspi_LoadBlob_UINT32(offset, pk->capitalR0Length, blob);
	Trspi_LoadBlob(offset, pk->capitalR0Length, blob, pk->capitalR0);

	Trspi_LoadBlob_UINT32(offset, pk->capitalR1Length, blob);
	Trspi_LoadBlob(offset, pk->capitalR1Length, blob, pk->capitalR1);

	Trspi_LoadBlob_UINT32(offset, pk->gammaLength, blob);
	Trspi_LoadBlob(offset, pk->gammaLength, blob, pk->gamma);

	Trspi_LoadBlob_UINT32(offset, pk->capitalGammaLength, blob);
	Trspi_LoadBlob(offset, pk->capitalGammaLength, blob, pk->capitalGamma);

	Trspi_LoadBlob_UINT32(offset, pk->rhoLength, blob);
	Trspi_LoadBlob(offset, pk->rhoLength, blob, pk->rho);

	for (i = 0; i < pk->capitalYLength; i++)
		Trspi_LoadBlob(offset, pk->capitalYLength2, blob, pk->capitalY[i]);

	Trspi_LoadBlob_UINT32(offset, pk->capitalYPlatformLength, blob);

	Trspi_LoadBlob_UINT32(offset, pk->issuerBaseNameLength, blob);
	Trspi_LoadBlob(offset, pk->issuerBaseNameLength, blob, pk->issuerBaseName);
}

TSS_RESULT
Trspi_UnloadBlob_DAA_PK(UINT64 *offset, BYTE *blob, TSS_DAA_PK *pk)
{
	UINT32 i = 0, j;

	memset(pk, 0, sizeof(TSS_DAA_PK));

	Trspi_UnloadBlob_TSS_VERSION(offset, blob, &pk->versionInfo);

	Trspi_UnloadBlob_UINT32(offset, &pk->modulusLength, blob);
	if (pk->modulusLength > 0) {
		if ((pk->modulus = malloc(pk->modulusLength)) == NULL)
			return TSPERR(TSS_E_OUTOFMEMORY);

		Trspi_UnloadBlob(offset, pk->modulusLength, blob, pk->modulus);
	} else {
		pk->modulus = NULL;
	}

	Trspi_UnloadBlob_UINT32(offset, &pk->capitalSLength, blob);
	if (pk->capitalSLength > 0) {
		if ((pk->capitalS = malloc(pk->capitalSLength)) == NULL)
			goto error;

		Trspi_UnloadBlob(offset, pk->capitalSLength, blob, pk->capitalS);
	} else {
		pk->capitalS = NULL;
	}

	Trspi_UnloadBlob_UINT32(offset, &pk->capitalZLength, blob);
	if (pk->capitalZLength > 0) {
		if ((pk->capitalZ = malloc(pk->capitalZLength)) == NULL)
			goto error;

		Trspi_UnloadBlob(offset, pk->capitalZLength, blob, pk->capitalZ);
	} else {
		pk->capitalZ = NULL;
	}

	Trspi_UnloadBlob_UINT32(offset, &pk->capitalR0Length, blob);
	if (pk->capitalR0Length > 0) {
		if ((pk->capitalR0 = malloc(pk->capitalR0Length)) == NULL)
			goto error;

		Trspi_UnloadBlob(offset, pk->capitalR0Length, blob, pk->capitalR0);
	} else {
		pk->capitalR0 = NULL;
	}

	Trspi_UnloadBlob_UINT32(offset, &pk->capitalR1Length, blob);
	if (pk->capitalR1Length > 0) {
		if ((pk->capitalR1 = malloc(pk->capitalR1Length)) == NULL)
			goto error;

		Trspi_UnloadBlob(offset, pk->capitalR1Length, blob, pk->capitalR1);
	} else {
		pk->capitalR1 = NULL;
	}

	Trspi_UnloadBlob_UINT32(offset, &pk->gammaLength, blob);
	if (pk->gammaLength > 0) {
		if ((pk->gamma = malloc(pk->gammaLength)) == NULL)
			goto error;

		Trspi_UnloadBlob(offset, pk->gammaLength, blob, pk->gamma);
	} else {
		pk->gamma = NULL;
	}

	Trspi_UnloadBlob_UINT32(offset, &pk->capitalGammaLength, blob);
	if (pk->capitalGammaLength > 0) {
		if ((pk->capitalGamma = malloc(pk->capitalGammaLength)) == NULL)
			goto error;

		Trspi_UnloadBlob(offset, pk->capitalGammaLength, blob, pk->capitalGamma);
	} else {
		pk->capitalGamma = NULL;
	}

	Trspi_UnloadBlob_UINT32(offset, &pk->rhoLength, blob);
	if (pk->rhoLength > 0) {
		if ((pk->rho = malloc(pk->rhoLength)) == NULL)
			goto error;

		Trspi_UnloadBlob(offset, pk->rhoLength, blob, pk->rho);
	} else {
		pk->rho = NULL;
	}

	Trspi_UnloadBlob_UINT32(offset, &pk->capitalYLength, blob);
	Trspi_UnloadBlob_UINT32(offset, &pk->capitalYLength2, blob);

	if (pk->capitalYLength > 0 && pk->capitalYLength2 > 0) {
		if ((pk->capitalY = calloc(pk->capitalYLength, sizeof(BYTE *))) == NULL)
			goto error;

		for (i = 0; i < pk->capitalYLength; i++) {
			if ((pk->capitalY[i] = malloc(pk->capitalYLength2)) == NULL)
				goto error;

			Trspi_UnloadBlob(offset, pk->capitalYLength2, blob, pk->capitalY[i]);
		}
	} else {
		pk->capitalY = NULL;
	}

	Trspi_UnloadBlob_UINT32(offset, &pk->capitalYPlatformLength, blob);

	Trspi_UnloadBlob_UINT32(offset, &pk->issuerBaseNameLength, blob);
	if (pk->issuerBaseNameLength > 0) {
		if ((pk->issuerBaseName = malloc(pk->issuerBaseNameLength)) == NULL)
			goto error;

		Trspi_UnloadBlob(offset, pk->issuerBaseNameLength, blob, pk->issuerBaseName);
	} else {
		pk->issuerBaseName = NULL;
	}

	return TSS_SUCCESS;

error:
	free(pk->modulus);
	free(pk->capitalS);
	free(pk->capitalZ);
	free(pk->capitalR0);
	free(pk->capitalR1);
	free(pk->gamma);
	free(pk->capitalGamma);
	free(pk->rho);
	if (pk->capitalY) {
		for (j = 0; j < i; j++)
			free(pk->capitalY[j]);

		free(pk->capitalY);
	}
	free(pk->issuerBaseName);

	memset(pk, 0, sizeof(TSS_DAA_PK));

	return TSPERR(TSS_E_OUTOFMEMORY);
}
