
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "daa_structs.h"
#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "spi_internal_types.h"
#include "spi_utils.h"
#include "obj.h"
#include "tsplog.h"
#include "daa_parameter.h"
#include "verifier.h"
#include "platform.h"

// for RSA Key
#include <openssl/rsa.h>

#define DEFAULT_FILENAME "issuer.txt"
#define DEFAULT_CREDENTIAL_FILENAME "credential.txt"
#define DEFAULT_DAACOUNTER 0x01020304
#define DEFAULT_OWN_PASSWD "OWN_PWD"

// from IssuerFactory
static const int DEFAULT_KEY_CHAIN_LENGTH = 3;

typedef struct tdIssuer {
	// use on Tspi calls
	TSS_DAA_PK *pk_extern;
	TSS_DAA_KEY_PAIR *key_pair_extern;

	// used internally
	int length_key_chain;
	RSA **key_chain;
	TSS_DAA_PK_internal *pk;
	DAA_PRIVATE_KEY_internal *private_key;
	TSS_DAA_PK_PROOF_internal *pk_proof;
	//RSA **auth_key_pairs;
	BYTE **pk_signatures;
	bi_ptr zeta;
} Issuer;

void *alloc( UINT32 length, TCS_CONTEXT_HANDLE tcsContext) {
	void *result = calloc_tspi( tcsContext, length);
	LogDebug("allocate tspi memory:%d", (int)result);
	return result;
}

/**
Used by RSA_generate_key
From RSA_generate_key documentation:
-> callback(2, n, cb_arg) is called when n-th randomly generated prime is rejected
-> callback(3, 0, cb_arg) is called when p is found with p-1 more or less prime to e
-> callback(3, 1, cb_arg) repeatedly called for prime q
*/
void callback(int step, int number, void *arg) {
#ifdef DAA_DEBUG
	putc( '.', stdout); fflush( stdout);
#endif
}

int sign( BYTE *buffer_2_sign,
	int len_buffer_2_sign,
	RSA *rsa,
	BYTE *signature,
	int *len_signature
) {
	EVP_MD_CTX *ctx;
	int len_message = EVP_MD_size( EVP_sha1()), current_len_message;
	BYTE *message = (BYTE *)malloc( len_message);
	int ret;

	ctx = EVP_MD_CTX_create();
	EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
	EVP_DigestUpdate(ctx, buffer_2_sign, len_buffer_2_sign);
	EVP_DigestFinal_ex(ctx, message, &current_len_message);
	LogDebug("Sign rsa-> with message (length=%d)", current_len_message);
//	int RSA_sign(int type, unsigned char *m, unsigned int m_len,
//		     unsigned char *sigret, unsigned int *siglen, RSA *rsa);
	ret = RSA_sign( NID_sha1, message, current_len_message, signature, len_signature, rsa);
	if( ret == 0) {
		LogError("Error in RSA_sign: %s", ERR_error_string( ERR_get_error(), NULL));
	}
	LogDebug("Sign rsa-> signature (length=%d)", *len_signature );
	EVP_MD_CTX_destroy(ctx);
	free( message);
	return ret;
}

/* Compute key chain. */
static int init_key_chain(int length_key_chain, Issuer *issuer) {
	BYTE *signature;
	int i, len_sign, ret;
	BYTE *modulus;
	BYTE *message;
	// generate RSA key of length  DAA_PARAM_KEY_SIZE with exponent
	// 65537 (java.security.spec.RSAKeyGenParameterSpec.F4)
	unsigned long e = 65537;
	RSA *rsa;
	bi_ptr bi;
	EVP_MD_CTX *ctx;
	int len_message = EVP_MD_size( EVP_sha1());
	int current_len_message;

	EVP_MD_CTX_create(ctx);
	message = (BYTE *)malloc(len_message);
	if( length_key_chain < 1) {
		free( message);
		return -1;
	}
	issuer->length_key_chain = length_key_chain;
	issuer->key_chain = (RSA **)malloc(sizeof(RSA *) * length_key_chain);
	issuer->pk_signatures = (BYTE **)malloc(sizeof(BYTE *) * length_key_chain);
	for(i = 0; i<length_key_chain; i++) {
		rsa = RSA_generate_key( DAA_PARAM_KEY_SIZE, e, &callback, NULL);
		if( (BN_num_bits(rsa->n) + 7) / 8 != (DAA_PARAM_KEY_SIZE + 7) / 8) {
			LogError("BN_num_bits(rsa->n) + 7) / 8 != (DAA_PARAM_KEY_SIZE + 7) / 8)");
			return -1;
		}
		issuer->key_chain[i] = rsa;
		if( i > 0) {
			signature = (BYTE *)malloc( RSA_size(rsa));
			modulus = (BYTE *)malloc( DAA_PARAM_KEY_SIZE / 8);
			// signature algorithm from Issuer.java -  "SHA1withRSA"
			// sign the modulus (n) of the RSA key with the previous RSA key (chain)
			// 	sign rsa(i)->n with auth_key_pairs[i-1]
			LogDebug("modulus=%s\n", dump_byte_array(256, modulus));
			LogDebug("signature=%s\n", dump_byte_array(256, signature));
			bi = bi_new_ptr();
			bi_set_as_hex( bi, BN_bn2hex( rsa->n));
			bi_2_byte_array( modulus, DAA_PARAM_KEY_SIZE / 8, bi);
			LogDebug("bi=%s", bi_2_hex_char( bi));
			bi_free_ptr(  bi);
			EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
			EVP_DigestUpdate(ctx, modulus, DAA_PARAM_KEY_SIZE / 8);
			EVP_DigestFinal_ex(ctx, message, &current_len_message);
			ret = RSA_sign( NID_sha1, message, current_len_message,
					signature, &len_sign, issuer->key_chain[i-1]);
			if( ret == 0) {
				LogError("Error in RSA_sign: %s",
					ERR_error_string( ERR_get_error(), NULL));
			}
			LogDebug("Sign rsa->n (length=%d) with signature (length=%d,\
 truelength=%d message_len=%d) ret = %d ERROR?=%s",
				RSA_size(rsa),
				DAA_PARAM_KEY_SIZE / 8,
				len_sign,
				current_len_message,
				ret,
				ERR_error_string( ERR_get_error(),
				NULL) );
			LogDebug("message=%s\n", dump_byte_array(256, message));
			LogDebug("signature=%s\n",dump_byte_array(256, signature));
			issuer->pk_signatures[i-1] = signature;
		}
	}
	free( message);
	EVP_MD_CTX_destroy(ctx);
	return 0;
}

Issuer* initIssuer(int length_key_chain, char *filename, char *exec, TSS_HCONTEXT hContext) {
	FILE *file;
	EVP_MD_CTX *mdctx;
	Issuer *issuer = (Issuer *)malloc(sizeof( Issuer));
	TPM_DAA_ISSUER *tpm_daa_issuer;
	bi_ptr modulus_N0;
	int len_issuer_settings, len_signature;
	BYTE *modulus_N0_bytes;
	BYTE *digest_n0;
	BYTE *issuer_settings_byte_array;
	BYTE *sign_data;
	BYTE *signature;
	RSA *private_nn;
	KEY_PAIR_WITH_PROOF_internal *key_pair_with_proof;

	LogDebug("Loading issuer info (keypair & proof) -> \'%s\'", filename);
	file = fopen( filename, "r");
	if( file == NULL) {
		fprintf( stderr, "%s: Error when opening \'%s\': %s\n",
			exec,
			filename,
			strerror( errno));
		free( issuer);
		return NULL;
	}
	key_pair_with_proof = load_KEY_PAIR_WITH_PROOF( file);
	if( key_pair_with_proof == NULL) {
		LogError( "Error when reading \'%s\': %s\n", filename, strerror( errno));
		free( issuer);
		return NULL;
	}
	fclose( file);
	issuer->pk = key_pair_with_proof->pk;
	issuer->pk_extern = i_2_e_TSS_DAA_PK( issuer->pk, &alloc, hContext);
	issuer->key_pair_extern = (TSS_DAA_KEY_PAIR *)malloc( sizeof(TSS_DAA_KEY_PAIR));
	init_tss_version( issuer->key_pair_extern);
	issuer->key_pair_extern->public_key = issuer->pk_extern;
	issuer->key_pair_extern->private_key = i_2_e_TSS_DAA_PRIVATE_KEY(
		key_pair_with_proof->private_key,
		&alloc, hContext);
	issuer->pk_proof = key_pair_with_proof->proof;
	issuer->private_key = key_pair_with_proof->private_key;
	init_key_chain( length_key_chain, issuer);
	issuer->zeta = compute_zeta( issuer->pk->issuerBaseNameLength,
					issuer->pk->issuerBaseName, issuer->pk);
	// sign "issuer settings"
	modulus_N0 = bi_new_ptr();
	bi_set_as_hex( modulus_N0, BN_bn2hex( issuer->key_chain[0]->n));
	// in TPM, N0 is hashed by hashing the scratch (256 bytes) so it must
	// be formatted according to the scratch size (TPM_DAA_SIZE_issuerModulus)
	modulus_N0_bytes = (BYTE *)malloc( TPM_DAA_SIZE_issuerModulus);
	bi_2_byte_array( modulus_N0_bytes, TPM_DAA_SIZE_issuerModulus, modulus_N0);
	bi_free_ptr( modulus_N0);

	if( TPM_DAA_SIZE_issuerModulus * 8 != DAA_PARAM_KEY_SIZE) {
		LogError("TPM_DAA_SIZE_issuerModulus * 8 (%d) != DAA_PARAM_KEY_SIZE(%d)",
			TPM_DAA_SIZE_issuerModulus*8,
			DAA_PARAM_KEY_SIZE);
		free( issuer);
		return NULL;
	}

	EVP_MD_CTX_create(mdctx);
	EVP_DigestInit_ex(mdctx, DAA_PARAM_get_message_digest(), NULL);
	// digestN0 = hash( modulus_N0)
	EVP_DigestUpdate(mdctx,  modulus_N0_bytes, TPM_DAA_SIZE_issuerModulus);
	digest_n0 = (BYTE *)EVP_MD_CTX_create();
	EVP_DigestFinal_ex(mdctx, digest_n0, NULL);
	tpm_daa_issuer = convert2issuer_settings( issuer->pk);
	issuer_settings_byte_array = issuer_2_byte_array( tpm_daa_issuer, &len_issuer_settings);
	// data to sign: concatenation of digest_n0 and issuer_settings_byte_array
	sign_data = (BYTE *)malloc( EVP_MD_CTX_size(mdctx) + len_issuer_settings);
	memcpy( sign_data, digest_n0, EVP_MD_CTX_size(mdctx));
	memcpy( &sign_data[EVP_MD_CTX_size(mdctx)],
			issuer_settings_byte_array,
			len_issuer_settings);
	free( issuer_settings_byte_array);
	// sign digest of TPM compatible Issuer key (sign_data)
	private_nn = issuer->key_chain[issuer->length_key_chain - 1];
	signature = (BYTE *)malloc( RSA_size(private_nn));
	if ( sign( sign_data, EVP_MD_CTX_size(mdctx) + len_issuer_settings,
			private_nn,
			signature,
			&len_signature) ==0) {
		LogError("Can not sign digest of TPM compatible Issuer key");
		goto close;
	}
	issuer->pk_signatures[ issuer->length_key_chain - 1] = signature;
	LogDebug("Set last signature sign[%d] = %s",
			issuer->length_key_chain - 1,
			dump_byte_array(EVP_MD_size( EVP_sha1()),
			signature));
	// TODO sign the complete public key of TPM compatible Issuer key
/*	EVP_DigestInit_ex(mdctx, DAA_PARAM_get_message_digest(), NULL);
	EVP_DigestUpdate(mdctx, digest_n0, EVP_MD_CTX_size(mdctx));
	pk_encoded = encoded_DAA_PK_internal( &pk_encodedLength, issuer->pk);
	EVP_DigestUpdate(mdctx,  pk_encoded, pk_encodedLength);
	EVP_DigestFinal(mdctx, , NULL);
	signature = (BYTE *)malloc( EVP_MD_size( EVP_sha1()));
	if (sign( sign_data, EVP_MD_CTX_size(mdctx) + len_issuer_settings,
		private_nn, signature, &len_signature) !=0) goto close;
*/
close:
	EVP_MD_CTX_destroy(mdctx);
	free( digest_n0);
	free( sign_data);
	return issuer;
}

int print_usage(char  *exec) {
	fprintf(stderr, "usage: %s\n", exec);
	fprintf(stderr, "\t-if,\t--issuer_file\n\t\tthe file that will contain all key\
pair and proof to be used by the issuer\n\t\t (default: %s)\n",
		DEFAULT_FILENAME);
	fprintf(stderr, "\t-dc,\t--daa_counter\n\t\tdaa counter (default: %d)\n",
		DEFAULT_DAACOUNTER);
	fprintf(stderr,
		"\t-pw,\t--passwd\n\t\ttpm owner password (default: %s)\n",
		DEFAULT_OWN_PASSWD);
	return -1;
}

int main(int argc, char *argv[]) {
	TSS_HCONTEXT hContext;
	TSS_RESULT result;
	TSS_HTPM hTPM;
	TSS_HPOLICY hPolicy;
	int i, length;
	char *param, *filename = DEFAULT_FILENAME;
	char *credential_filename = DEFAULT_CREDENTIAL_FILENAME;
	UINT32 daaCounter = DEFAULT_DAACOUNTER;
	UINT32 capital_UPrimeLength;
	BYTE *capitalUPrime;
	TSS_DAA_IDENTITY_PROOF identityProof;
	TSS_DAA_JOIN_SESSION joinSession;
	TSS_DAA_JOIN_ISSUER_SESSION join_issuer_session;
	TSS_DAA_CREDENTIAL_REQUEST credentialRequest;
	TSS_DAA_CRED_ISSUER credIssuer;
	TSS_HDAA hDAA;
	Issuer* issuer;
	char *szTpmPasswd = DEFAULT_OWN_PASSWD;
	UINT32 endorsementKeyLength;
	BYTE *endorsementKey;
	UINT32 nonceIssuerLength;
	BYTE *nonceIssuer;
	UINT32 authenticationChallengeLength;
	BYTE *authenticationChallenge;
	bi_array_ptr capital_receiver;
	BYTE **attributesPlatform;
	UINT32 attributesPlatformLength;
	BYTE **attributesIssuer;
	UINT32 attributesIssuerLength;
	bi_t random;
	FILE *file;

	init_tss_version( &identityProof);
	init_tss_version( &joinSession);
	init_tss_version( &join_issuer_session);
	init_tss_version( &credentialRequest);
	init_tss_version( &credIssuer);
	i = 1;
	while( i < argc) {
		param = argv[ i];
		if ( strcmp( param, "-if") == 0 || strcmp( param, "--issuer_file") == 0) {
			i++;
			if( i == argc) return print_usage( argv[0]);
			filename = argv[i];
		} else if( strcmp( param, "-dc") == 0 || strcmp( param, "--daa_counter") == 0){
			i++;
			if( i == argc) return print_usage( argv[0]);
			daaCounter = atoi(argv[i]);
		} else if( strcmp( param, "-pw") == 0 || strcmp( param, "--passwd") == 0){
			i++;
			if( i == argc) return print_usage( argv[0]);
			szTpmPasswd = argv[i];
		} else {
			fprintf(stderr, "\n%s:unrecognized option <%s>\n", argv[0], param);
			return print_usage( argv[0]);
		}
		i++;
	}
	bi_init( NULL);

	// Create Context
	LogDebug("Create Context");
	result = Tspi_Context_Create( &hContext );
	if ( result != TSS_SUCCESS )
	{
		LogError( "Tspi_Context_Create %d\n", result );
		goto out;
	}
	// Connect to Context
	result = Tspi_Context_Connect( hContext, NULL );
	if ( result != TSS_SUCCESS) goto out_close;
	printf("\nConnect to the context: %X\n", hContext);

	if( (result = Tspi_Context_GetTpmObject( hContext, &hTPM)) != TSS_SUCCESS)
		goto out_close;
	// Get the correct policy using the TPM ownership PASSWD
	if( (result = Tspi_GetPolicyObject( hTPM,
						TSS_POLICY_USAGE,
						&hPolicy)) != TSS_SUCCESS)
		goto out_close;
	if( (result = Tspi_Policy_SetSecret( hPolicy,
						TSS_SECRET_MODE_PLAIN,
						strlen( szTpmPasswd),
						szTpmPasswd)) != TSS_SUCCESS)
		goto out_close;
	LogDebug("Tspi_Policy_SetSecret hPolicy received;%d\n", hPolicy);

	//Create Object
	result = obj_daa_add( hContext, &hDAA);
	if (result != TSS_SUCCESS) {
		LogError("Tspi_Context_CreateObject:%d\n", result);
		Tspi_Context_Close(hContext);
		LogError("%s: %s\n", argv[0], err_string(result));
		exit(result);
	}
	LogDebug("created DAA object:%X", hDAA);
	issuer = initIssuer( DEFAULT_KEY_CHAIN_LENGTH, filename, argv[0], hContext);
	if( issuer == NULL) goto out_close;

	// generate receiver attributes and issuer attributes (random)
	attributesPlatformLength = issuer->pk->capitalRReceiver->length;
	attributesPlatform = (BYTE **)malloc( attributesPlatformLength * sizeof(BYTE *));
	bi_new( random);
	for( i=0; i<(int)attributesPlatformLength; i++) {
		bi_urandom( random, DAA_PARAM_SIZE_F_I);
		attributesPlatform[i] = bi_2_nbin( &length, random);
		if( attributesPlatform[i] == NULL) {
			LogError("malloc of %d bytes failed", length);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto out_close;
		}
	}
	attributesIssuerLength = issuer->pk->capitalRIssuer->length;
	attributesIssuer = (BYTE **)malloc( attributesIssuerLength * sizeof(BYTE *));
	for( i=0; i<(int)attributesIssuerLength; i++) {
		bi_urandom( random, DAA_PARAM_SIZE_F_I);
		attributesIssuer[i]  = bi_2_nbin( &length, random);
		if( attributesIssuer[i] == NULL) {
			LogError("malloc of %d bytes failed", length);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto out_close;
		}
	}
	bi_free(random);
	LogDebug("Generated attributes (Platform=%d,Issuer=%d)",
		attributesPlatformLength, attributesIssuerLength);

	result = Tspi_TPM_DAA_JoinInit(
		hDAA,	// in
		hTPM,	// in
		daaCounter,	// in
		(TSS_HKEY)issuer->pk_extern,	// in
		issuer->length_key_chain,	// in
		(TSS_HKEY *)issuer->key_chain,	// in
		issuer->length_key_chain,	// in
		issuer->pk_signatures,	// in
		&capital_UPrimeLength,	// out
		&capitalUPrime,	// out
		&identityProof,	// out
		&joinSession	// out
	);
	if( result != TSS_SUCCESS) goto out_close;

	result = Tspi_DAA_IssueInit(
		hDAA,	// in
		(TSS_HKEY)issuer->key_chain[0],	// in
		(TSS_HKEY)issuer->key_pair_extern,	// in
		identityProof,	// in
		capital_UPrimeLength,	// in
		capitalUPrime,	// in
		daaCounter,	// in
		&nonceIssuerLength,	// out
		&nonceIssuer,	// out
		&authenticationChallengeLength,	// out
		&authenticationChallenge,	// out
		&join_issuer_session	// out
	);
	if( result != TSS_SUCCESS) goto out_close;

	result = Tspi_TPM_DAA_JoinCreateDaaPubKey(
		hDAA,	// in
		hTPM,	// in
		authenticationChallengeLength,	// in
		authenticationChallenge,	// in
		nonceIssuerLength,	// in
		nonceIssuer,	// in
		attributesPlatformLength,	// in
		attributesPlatform,	// in
		&joinSession,	// in & out
		&credentialRequest	// out
	);
	if( result != TSS_SUCCESS) goto out_close;

	result = Tspi_DAA_IssueCredential(
		hDAA,	// in
		attributesIssuerLength,	// in
		attributesIssuer,	// in
		credentialRequest,	// in
		join_issuer_session,	// in
		&credIssuer	// out
	);

	result = Tspi_TPM_DAA_JoinStoreCredential(
		hDAA,	// in
		hTPM,	// in
		credIssuer,	// in
		joinSession,	// in
		(TSS_HKEY*)&credentialRequest	// out
	);
	if( result != TSS_SUCCESS) goto out_close;

	printf("Saving credential: %s ", credential_filename);
	file = fopen( credential_filename, "w");
	if( save_TSS_DAA_CREDENTIAL( file, &credentialRequest) != 0) {
		LogError( "[test_join]: Error when saving \'%s\': %s",
			credential_filename,
			strerror( errno));
		result = TSS_E_FAIL;
		goto out_close;
	}
	fclose( file);
	printf("Done\n");

out_close:
	Tspi_Context_FreeMemory( hContext, NULL );
	Tspi_Context_Close( hContext );
out:
	bi_release();
	LogDebug("THE END result=%d:%s",result, err_string( result) );;
	return result;
}
