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

#include "trousers/tss.h"
#include "spi_internal_types.h"
#include "spi_utils.h"
#include "obj.h"
// #include "tcslog.h"
#include "bi.h"
#include "daa_parameter.h"
#include "issuer.h"

static char *DEFAULT_FILENAME = "issuer.txt";
static char *DEFAULT_ISSUER = "IBM-Issuer";

static const int DEFAULT_ISSUER_ATTRIBUTES = 2;	// A1 A2
static const int DEFAULT_RECEIVER_ATTRIBUTES = 3;	// A3 A4 A5

int print_usage(char *cmd) {
	fprintf(stderr, "usage: %s\n", cmd);
	fprintf(stderr, "	\t-npa,\t--nb_platform_attr\tnumber of attributes that the\
 Platform can choose and which will not be visible to the Issuer (default: %d)\n",
			DEFAULT_ISSUER_ATTRIBUTES);
	fprintf(stderr, "	\t-nia,\t--nb_issuer_attr\tnumber of attributes that the issuer\
 can choose and which will be visible to both the Platform and the Issuer(default: %d)\n",
			DEFAULT_RECEIVER_ATTRIBUTES);
	fprintf(stderr, "	\t-if,\t--issuer_file\tthe file that will contain all key pair\
 and proof to be used by the issuer (default: %s)\n",
			DEFAULT_FILENAME);
	fprintf(stderr, "	\t-i,\t--issuer\tissuer identity (default: %s)\n",
			DEFAULT_ISSUER);
	return -1;
}

int main(int argc, char *argv[]) {
	int nb_platform_attr = DEFAULT_ISSUER_ATTRIBUTES;
	int nb_issuer_attr = DEFAULT_RECEIVER_ATTRIBUTES;
	char *filename = DEFAULT_FILENAME;
	char *issuer = DEFAULT_ISSUER;
	int i;
	char *param;
	TSS_HCONTEXT hContext;
	TSS_DAA_KEY_PAIR *key_pair;
	TSS_DAA_PK_PROOF *public_keyproof;
	TSS_RESULT result;
	TSS_HDAA hDAA;
	TSS_DAA_PK_PROOF_internal *public_keyproof_internal;
	TSS_DAA_PK_internal *pk;
	TSS_DAA_PRIVATE_KEY *private_key;
	DAA_PRIVATE_KEY_internal *private_key_internal;
	KEY_PAIR_WITH_PROOF_internal *key_pair_with_proof;

	printf("Issuer Setup (%s:%s,%s)\n", argv[0], __DATE__, __TIME__);
	i = 1;
	while( i < argc) {
		param = argv[ i];
		if         ( strcmp( param, "-if") == 0 || strcmp( param, "--issuer_file")) {
			i++;
			if( i == argc) return print_usage( argv[0]);
			filename = argv[i];
		} else if( strcmp( param, "-npa") == 0 || strcmp( param, "--nb_platform_attr")) {
			i++;
			if( i == argc) return print_usage( argv[0]);
			nb_platform_attr = atoi( argv[i]);
		} else if( strcmp( param, "-nia") == 0 || strcmp( param, "--nb_issuer_attr")) {
			i++;
			if( i == argc) return print_usage( argv[0]);
			nb_issuer_attr = atoi(argv[i]);
		} else if( strcmp( param, "-i") == 0 || strcmp( param, "--issuer")) {
			i++;
			if( i == argc) return print_usage( argv[0]);
			issuer = argv[i];
		} else {
			fprintf(stderr, 	"%s:unrecognized option `%s'\n", argv[0], param);
			return print_usage( argv[0]);
		}
		i++;
	}
	bi_init( NULL);
	// Create Context
	printf("Create Context\n");
	result = Tspi_Context_Create( &hContext );
	if ( result != TSS_SUCCESS )
	{
		fprintf( stderr, "Tspi_Context_Create %d\n", result );
		exit( result );
	}

	// Connect to Context
	printf("Connect to the context\n");
	result = Tspi_Context_Connect( hContext, NULL );
	if ( result != TSS_SUCCESS )
	{
		fprintf( stderr, "Tspi_Context_Connect error:%d\n", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}
	//Create Object
	result = obj_daa_add( hContext, &hDAA);
	if (result != TSS_SUCCESS) {
		goto close;
	}
	result = Tspi_DAA_IssueSetup(
		hDAA,	// in
		strlen( issuer),	// in
		(BYTE *)issuer,	// in
		nb_platform_attr,	// in
		nb_issuer_attr,	// in
		(TSS_HKEY *)&key_pair,	// out
		&public_keyproof);	// out
	if( result != TSS_SUCCESS) goto close;

	// TSS_DAA_KEY_PAIR_internal *key_pair_internal = DAA_KEY_PAIR_2_internal( key_pair);
	public_keyproof_internal = e_2_i_TSS_DAA_PK_PROOF( public_keyproof);
	pk = e_2_i_TSS_DAA_PK( key_pair->public_key);
	private_key = key_pair->private_key;
	private_key_internal = e_2_i_TSS_DAA_PRIVATE_KEY( private_key);
	key_pair_with_proof =
		(KEY_PAIR_WITH_PROOF_internal *)malloc( sizeof(KEY_PAIR_WITH_PROOF_internal));
	if( key_pair_with_proof == NULL) {
		fprintf("malloc of %d bytes failed", sizeof(KEY_PAIR_WITH_PROOF_internal));
		goto close;
	}
	key_pair_with_proof->pk = pk;
	key_pair_with_proof->private_key = private_key_internal;
	key_pair_with_proof->proof = public_keyproof_internal;

	printf("Saving key pair with proof  -> 	\'%s\'", filename);
	FILE *file = fopen( filename, "w");
	if( file == NULL) {
		fprintf( stderr, "%s: Error when saving \'%s\': %s\n",
			argv[0],
			filename,
			strerror( errno));
		return -1;
	}
	if( save_KEY_PAIR_WITH_PROOF( file, key_pair_with_proof) != 0) {
		fprintf( stderr, "%s: Error when saving \'%s\': %s\n",
			argv[0],
			filename,
			strerror( errno));
		return -1;
	}
	fclose( file);
	printf("\nDone.\n");
close:
	obj_daa_remove( hDAA, hContext);
	printf("Closing the context\n");
	Tspi_Context_FreeMemory( hContext, NULL );
	Tspi_Context_Close( hContext );
	bi_release();
	printf("Result: %d", result);
	return result;
}
