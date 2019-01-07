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

#include <limits.h>
#include <ctype.h>

#include "tpm_nvcommon.h"
#include "tpm_tspi.h"
#include "tpm_utils.h"

const struct strings_with_values permvalues[] = {
	{
		.name = "AUTHREAD",
		.value = TPM_NV_PER_AUTHREAD,
		.desc = _("reading requires NVRAM area authorization"),
	},
	{
		.name = "AUTHWRITE",
		.value = TPM_NV_PER_AUTHWRITE,
		.desc = _("writing requires NVRAM area authorization"),
	},
	{
		.name = "OWNERREAD",
		.value = TPM_NV_PER_OWNERREAD,
		.desc = _("writing requires owner authorization"),
	},
	{
		.name = "OWNERWRITE",
		.value = TPM_NV_PER_OWNERWRITE,
		.desc = _("reading requires owner authorization"),
	},
	{
		.name = "PPREAD",
		.value = TPM_NV_PER_PPREAD,
		.desc = _("writing requires physical presence"),
	},
	{
		.name = "PPWRITE",
		.value = TPM_NV_PER_PPWRITE,
		.desc = _("reading requires physical presence"),
	},
	{
		.name = "GLOBALLOCK",
		.value = TPM_NV_PER_GLOBALLOCK,
		.desc = _("write to index 0 locks the NVRAM area until "
		          "TPM_STARTUP(ST_CLEAR)"),
	},
	{
		.name = "READ_STCLEAR",
		.value = TPM_NV_PER_READ_STCLEAR,
		.desc = _("a read with size 0 to the same index prevents further "
		          "reading until ST_STARTUP(ST_CLEAR)"),
	},
	{
		.name = "WRITE_STCLEAR",
		.value = TPM_NV_PER_WRITE_STCLEAR,
		.desc = _("a write with size 0 to the same index prevents further "
                          "writing until ST_STARTUP(ST_CLEAR)"),
	},
	{
		.name = "WRITEDEFINE",
		.value = TPM_NV_PER_WRITEDEFINE,
		.desc = _("a write with size 0 to the same index locks the "
		          "NVRAM area permanently"),
	},
	{
		.name = "WRITEALL",
		.value = TPM_NV_PER_WRITEALL,
		.desc = _("the value must be written in a single operation"),
	},
	{
		.name = NULL,
	},
};


void displayStringsAndValues(const struct strings_with_values *svals,
                             const char *indent)
{
	unsigned int i;

	logMsg("%sThe following strings are available:\n", indent);
	for (i = 0; svals[i].name; i++)
		logMsg("%s%15s : %s\n", indent, svals[i].name, svals[i].desc);
}


int parseStringWithValues(const char *aArg,
			  const struct strings_with_values *svals,
			  unsigned int *x, unsigned int maximum,
			  const char *name)
{
	unsigned int offset = 0;
	int i, num, numbytes;
	size_t totlen = strlen(aArg);
	*x = 0;

	while (offset < totlen) {
		int found = 0;

		while (isspace(*(aArg + offset)))
			offset++;

		for (i = 0; svals[i].name; i++) {
			size_t len = strlen(svals[i].name);
			if (strncmp(aArg + offset, svals[i].name, len) == 0 &&
				    (aArg[offset+len] == '|' ||
				     aArg[offset+len] == 0)) {
				*x |= svals[i].value;
				offset += len + 1;
				found = 1;
				break;
			}
		}

		if (!found) {
			if (strncmp(aArg + offset, "0x", 2) == 0) {
				if (sscanf(aArg + offset, "%x%n", &num, &numbytes)
				    != 1) {
					logError(_("Could not parse hexadecimal "
					           "number in %s.\n"),
						 aArg);
					return -1;
				}
				if (aArg[offset+numbytes] == '|' ||
				    aArg[offset+numbytes] == 0) {
					logError(_("Illegal character following "
                                                   "hexadecimal number in %s\n"),
						 aArg + offset);
					return -1;
				}
				*x |= num;

				offset += numbytes + 1;
				found = 1;
			}
		}

		while (!found) {
			if (!isdigit(*(aArg+offset)))
				break;

			if (sscanf(aArg + offset, "%u%n", &num, &numbytes) != 1) {
				logError(_("Could not parse data in %s.\n"),
					 aArg + offset);
				return -1;
			}

			if (!aArg[offset+numbytes] == '|' &&
			    !aArg[offset+numbytes] == 0) {
				logError(_("Illegal character following decimal "
				           "number in %s\n"),
					 aArg + offset);
				return -1;
			}
			*x |= num;

			offset += numbytes + 1;
			found = 1;
			break;
		}

		if (!found) {
			logError(_("Unknown element in %s: %s.\n"),
			         name, aArg + offset);
			return -1;
		}
	}

	return 0;
}


char *printValueAsStrings(unsigned int value,
			  const struct strings_with_values *svals)
{
	unsigned int mask = (1 << 31), i,c;
	unsigned int buffer_size = 1024;
	char *buffer = calloc(1, buffer_size);
	char printbuf[30];
	size_t len;

	c = 0;

	while (mask && value) {
		if (value & mask) {
			for (i = 0; svals[i].name; i++) {
				if (svals[i].value == mask) {
					len = strlen(svals[i].name);
					if (len + strlen(buffer) + 1 >
					    buffer_size) {
						buffer_size += 1024;
						buffer = realloc(buffer,
								 buffer_size);
						if (!buffer)
							return NULL;
					}
					if (c)
						strcat(buffer, "|");
					strcat(buffer, svals[i].name);
					c ++;
					value ^= mask;
					break;
				}
			}
		}
		mask >>= 1;
	}
	if (value) {
		snprintf(printbuf, sizeof(printbuf), "%s0x%x",
		         c ? "|" : "",
		         value);
                len = strlen(printbuf);
		if (len + strlen(buffer) + 1 > buffer_size) {
			buffer_size += 1024;
			buffer = realloc(buffer,
					 buffer_size);
			if (!buffer)
				return NULL;
		}
		if (c)
			strcat(buffer, printbuf);
	}
	return buffer;
}


int parseHexOrDecimal(const char *aArg, unsigned int *x,
                      unsigned int minimum, unsigned int maximum,
		      const char *name)
{
	while (isspace(*aArg))
		aArg++;

	if (strncmp(aArg, "0x", 2) == 0) {
		if (sscanf(aArg, "%x", x) != 1) {
			return -1;
		}
	} else {
		if (!isdigit(*aArg)) {
		        fprintf(stderr,
		                "%s must be a positive integer.\n", name);
			return -1;
                }

		if (sscanf(aArg, "%u", x) != 1) {
			return -1;
		}
	}

	if ((*x > maximum) || (*x < minimum)) {
		fprintf(stderr, "%s is out of valid range [%u, %u]\n",
			name, minimum, maximum);
		return -1;
	}

	return 0;
}


TSS_RESULT getNVDataPublic(TSS_HTPM hTpm, TPM_NV_INDEX nvindex,
                           TPM_NV_DATA_PUBLIC **pub)
{
	TSS_RESULT res;
	UINT32 ulResultLen;
	BYTE *pResult;
	UINT64 off = 0;

	res = getCapability(hTpm, TSS_TPMCAP_NV_INDEX, sizeof(UINT32),
			    (BYTE *)&nvindex, &ulResultLen, &pResult);

	if (res != TSS_SUCCESS)
		return res;

	*pub = calloc(1, sizeof(TPM_NV_DATA_PUBLIC));
	if (*pub == NULL) {
		res = TSS_E_OUTOFMEMORY;
		goto err_exit;
	}

	res = unloadNVDataPublic(&off, pResult, ulResultLen, *pub);

	if (res != TSS_SUCCESS) {
		freeNVDataPublic(*pub);
		*pub = NULL;
	}
err_exit:
	return res;
}

void freeNVDataPublic(TPM_NV_DATA_PUBLIC *pub)
{
	if (!pub)
		return;

	free(pub->pcrInfoRead.pcrSelection.pcrSelect);
	free(pub->pcrInfoWrite.pcrSelection.pcrSelect);
	free(pub);
}
