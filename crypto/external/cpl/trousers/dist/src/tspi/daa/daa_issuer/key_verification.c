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
#include <errno.h>

#include <trousers/tss.h>
#include "spi_internal_types.h"
#include "spi_utils.h"
#include "obj.h"
#include "issuer.h"

static char *DEFAULT_FILENAME = "issuer.txt";

//static TSS_HCONTEXT _hContext;

static void *intern_alloc( size_t size, TSS_HOBJECT param_alloc) {
	// void *ret = calloc_tspi( , size);
	void *ret = malloc( size);
	LogDebug("[intern_alloc (%d)] -> %d", (int)size, (int)ret);
	return ret;
}

void isCorrect( TSS_HDAA hDAA,
			TSS_DAA_PK_internal *pk_internal,
			TSS_DAA_PK_PROOF_internal *proof_internal)
{
	TSS_BOOL isCorrect;
	TSS_RESULT	result;
	TSS_DAA_PK *pk;
	TSS_DAA_PK_PROOF *pk_proof;

	pk = i_2_e_TSS_DAA_PK( pk_internal, &intern_alloc, (TSS_HOBJECT)NULL);
	pk_proof = i_2_e_TSS_DAA_PK_PROOF( proof_internal,
					&intern_alloc,
					(TSS_HOBJECT)NULL);
	result = Tspi_DAA_IssuerKeyVerification( hDAA,
						(TSS_HKEY)pk,
						pk_proof,
						&isCorrect);
	if ( result != TSS_SUCCESS ) {
		fprintf( stderr, "Tspi_DAA_IssuerKeyVerification error: %d\n", result );
	}
	free_TSS_DAA_PK( pk);
	printf("isCorrect=%d\n", isCorrect);
}

int print_usage(char *cmd) {
	fprintf(stderr, "usage: %s\n", cmd);
	fprintf(stderr, "\t-if,\t--issuer_file\tthe file that will contain\
 all key pair and proof to be used by the issuer (default: %s)\n", DEFAULT_FILENAME);
	return -1;
}

int main(int argc, char *argv[]) {
	char *filename = DEFAULT_FILENAME;
	int i=1;
	char *param;
	TSS_RESULT	result;
	TSS_HCONTEXT	hContext;
	TSS_HDAA hDAA;
	FILE *file;

//	foreground = 1; // for debug
	printf("Key Verification (%s:%s,%s)\n", argv[0], __DATE__, __TIME__);
	while( i < argc) {
		param = argv[ i];
		if( strcmp( param, "-if") == 0 || strcmp( param, "--issuer_file")) {
			i++;
			if( i == argc) return print_usage( argv[0]);
			filename = argv[i];
		} else {
			fprintf(stderr, "%s:unrecognized option `%s'\n", argv[0], param);
			return print_usage( argv[0]);
		}
		i++;
	}
	bi_init( NULL);
	printf("Loading issuer info (keypair & proof) -> 	\'%s\'", filename);
	file = fopen( filename, "r");
	if( file == NULL) {
		fprintf( stderr,
			"%s: Error when opening \'%s\': %s\n",
			argv[0],
			filename,
			strerror( errno));
		return -1;
	}
	KEY_PAIR_WITH_PROOF_internal *key_pair_with_proof = load_KEY_PAIR_WITH_PROOF( file);
	if( key_pair_with_proof == NULL) {
		fprintf( stderr,
			"%s: Error when reading \'%s\': %s\n",
			argv[0],
			filename,
			strerror( errno));
		return -1;
	}
	fclose( file);

	// Create Context
	printf("\nCreate Context\n");
	result = Tspi_Context_Create( &hContext );
	if ( result != TSS_SUCCESS )
	{
		fprintf( stderr, "Tspi_Context_Create %d\n", result );
		exit( result );
	}

	// Connect to Context
	printf("\nConnect to the context\n");
	result = Tspi_Context_Connect( hContext, NULL );
	if ( result != TSS_SUCCESS )
	{
		fprintf( stderr, "Tspi_Context_Connect error:%d\n", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	//TODO save key in the persistent store
	// result = ps_write_key( fd, )

	//Create Object
	result = obj_daa_add( hContext, &hDAA);
	if (result != TSS_SUCCESS) {
		LogError("Tspi_Context_CreateObject:%d", result);
        	Tspi_Context_Close(hContext);
		LogError("issuer_setup: %s", err_string(result));
		exit(result);
	}

	// TSS_HDAA, TSS_HKEY, TSS_DAA_PK_PROOF, TSS_BOOL*
	isCorrect( hDAA, key_pair_with_proof->pk, key_pair_with_proof->proof);
	obj_daa_remove( hDAA, hContext);
	printf("\nClosing the context\n");
	Tspi_Context_FreeMemory( hContext, NULL );
	Tspi_Context_Close( hContext );
	exit( 0 );
}
