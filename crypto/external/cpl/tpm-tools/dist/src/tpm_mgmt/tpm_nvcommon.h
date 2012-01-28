/*
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2005 International Business
 * Machines Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Common Public License as published by
 * IBM Corporation; either version 1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Common Public License for more details.
 *
 * You should have received a copy of the Common Public License
 * along with this program; if not, a copy can be viewed at
 * http://www.opensource.org/licenses/cpl1.0.php.
 */
#ifndef TPM_NVCOMMON_H
#define TPM_NVCOMMON_H

#include "tpm_tspi.h"


struct strings_with_values
{
	const char *name;
	UINT32 value;
	const char *desc;
};

extern const struct strings_with_values permvalues[];

int parseStringWithValues(const char *aArg,
                          const struct strings_with_values *svals,
                          unsigned int *x, unsigned int maximum,
                          const char *name);

char *printValueAsStrings(unsigned int value,
                          const struct strings_with_values *svals);

int parseHexOrDecimal(const char *aArg, unsigned int *x,
                      unsigned int minimum, unsigned int maximum,
                      const char *name);

void displayStringsAndValues(const struct strings_with_values *svals, const char *indent);

TSS_RESULT getNVDataPublic(TSS_HTPM hTpm, TPM_NV_INDEX nvindex, TPM_NV_DATA_PUBLIC **pub);
void freeNVDataPublic(TPM_NV_DATA_PUBLIC *pub);

static inline UINT32
Decode_UINT32(BYTE * y)
{
        UINT32 x = 0;

        x = y[0];
        x = ((x << 8) | (y[1] & 0xFF));
        x = ((x << 8) | (y[2] & 0xFF));
        x = ((x << 8) | (y[3] & 0xFF));

        return x;
}

#endif /* TPM_NVCOMMON_H */
