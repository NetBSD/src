/*
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2005, 2006 International Business
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>

int main() {

	char startup[] = {
		0, 193,		/* TPM_TAG_RQU_COMMAND */
		0, 0, 0, 12,	/* length */
		0, 0, 0, 153,	/* TPM_ORD_Startup */
		0, 1		/* ST_CLEAR */
	};

	char selftest[] = {
		0, 193,		/* TPM_TAG_RQU_COMMAND */
		0, 0, 0, 10,	/* length */
		0, 0, 0, 80	/* TPM_ORD_SelfTestFull */
	};

	int fd;
	int err;
	int rc = 1;

	fd = open( "/dev/tpm0", O_RDWR );
	if ( fd < 0 ) {
		printf( "Unable to open the device.\n" );
		goto out_noclose;
	}

	err = write( fd, startup, sizeof(startup) );

	if ( err != sizeof( startup ) ){
		printf( "Error occured while writing the startup command: %d\n", errno );
		goto out;
	}
	
	err = read( fd, startup, sizeof(startup) );
	if ( err != 10 ) {
		printf( "Error occured while reading the startup result: %d %d %s\n", err, errno, strerror(errno));
		goto out;
	}

	err = ntohl( *((uint32_t *)(startup+6)) );
	if ( err == 38 ) {
		printf( "TPM already started.\n" );
		goto out;
	}

	if ( err != 0 ) {
		printf( "TPM Error occured: %d\n", err );
		goto out;
	}

	rc = 0;
	printf( "Startup successful\n" );

out:
	err = write( fd, selftest, sizeof(selftest) );

	if ( err != sizeof( selftest ) ){
		printf( "Error occured while writing the selftest command: %d\n", errno );
		goto out;
	}

	err = read( fd, selftest, sizeof(selftest) );
	if ( err != 10 ) {
		printf( "Error occured while reading the selftest result: %d %d %s\n", err, errno, strerror(errno));
		goto out;
	}

	err = ntohl( *((uint32_t *)(selftest+6)) );
	if ( err != 0 ) {
		printf( "TPM Error occured: %d\n", err );
		goto out;
	} else {
		rc = 0;
		printf( "Selftest successful\n" );
	}

	close(fd);

out_noclose:
        return rc;
}

