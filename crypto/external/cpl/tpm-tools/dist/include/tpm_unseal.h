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

#ifndef __TPM_UNSEAL_H
#define __TPM_UNSEAL_H

#define TPMSEAL_FILE_ERROR -2
#define TPMSEAL_STD_ERROR -1

enum tpm_errors {
	ENOTSSHDR = 0,
	ENOTSSFTR,
	EWRONGTSSTAG,
	EWRONGEVPTAG,
	EWRONGDATTAG,
	EWRONGKEYTYPE,
	EBADSEEK,
}; 

extern int tpm_errno;

int tpmUnsealFile(char*, unsigned char**, int*, BOOL);
void tpmUnsealShred(unsigned char*, int);
char* tpmUnsealStrerror(int);

#endif
