
/*
* Licensed Materials - Property of IBM
*
* trousers - An open source TCG Software Stack
*
* (C) Copyright International Business Machines Corp. 2006
*
*/

/*
This file implements Helper functions for converting / creating and freeing
internal representation of TSS_DAA structures.
An external representation is the one define in tss_structs.h.
An internal representation is using bi_t or bi_ptr for representing big numbers.
Naming convention: for each structures we can have:
init_(<STRUCT>, struct *) : initialize the version field
create_<STRUCT> : init all fields
free_<STRUCT> :  free all fields
e_2_i_<STRUCT> : convertor from External representation to internal
i_2_e_<STRUCT> : convertor from Internal to External. This call use a given memory
allocation function, to allow
for example to use calloc_tspi, or a "normal" malloc.
*/
#include <stdio.h>
#include <strings.h>
#include <errno.h>
#include <strings.h>

#include "daa_parameter.h"
#include "daa_structs.h"
#include "tcslog.h"

#define DUMP_DAA_PK_FIELD( field) \
do { \
	printf("%s=", #field); \
	dump_field( pk->field##Length, pk->field); \
	puts(""); \
} while(0);

#if 0
#define STORE_DAA_PK_BI1( field, bi)  \
do { \
	store_bi( &pk->field##Length, &pk->field, bi); \
} while(0);

#define STORE_DAA_PK_BI( field, daa_alloc, param_alloc)  \
do { \
	store_bi( &pk->field##Length,\
		&pk->field,\
		pk_internal->field,\
		daa_alloc,\
		param_alloc); \
} while(0);

// used only to read a structure from a file, so only as helping function
// for TCG application
static char buffer[1000];
#endif
BYTE *convert_alloc( TCS_CONTEXT_HANDLE tcsContext, UINT32 length, BYTE *source) {
	BYTE *result = calloc_tspi( tcsContext, length);

	if( result == NULL) return NULL;
	memcpy( result, source, length);
	free( source);
	return result;
}

BYTE *copy_alloc(  TCS_CONTEXT_HANDLE tcsContext, UINT32 length, BYTE *source) {
	BYTE *result = calloc_tspi( tcsContext, length);

	if( result == NULL) return NULL;
	memcpy( result, source, length);
	return result;
}

static void *normal_malloc( size_t size, TSS_HOBJECT object) {
	void *ret = malloc( size);
	return ret;
}

/* store a bi to a buffer and update the length in big endian format */
void store_bi( UINT32 *length,
		BYTE **buffer,
		const bi_ptr i,
		void * (*daa_alloc)(size_t size, TSS_HOBJECT object),
		TSS_HOBJECT object
) {
	int size;

	*buffer = (BYTE *)daa_alloc( bi_length( i), object);
	bi_2_nbin1( &size, *buffer, i);
	*length = size;
	LogDebug( "[store_bi] host_length:%d network_length:%d[address:%d]\n",
		size,
		(int)*length,
		(int)*buffer);
}

bi_ptr get_bi( const unsigned long n_length, const BYTE *buffer) {
	unsigned long length;

	length = n_length;
	LogDebug( "[get_bi] %d [address:%d -> |%2x|%2x| ]\n",
		(int)length,
		(int)buffer,
		(int)(buffer[0] &0xFF),
		(int)(buffer[1]&0xFF));
	return bi_set_as_nbin( length, buffer);
}

/* length is in network format: big indian */
void dump_field( int length, BYTE *buffer) {
	int i;

	for(  i=0; i< length; i++) {
		BYTE byte = (BYTE)(buffer[i] & 0xFF);
		printf("%02X", byte);
	}
}

#if 0
/* !: do not keep the return buffer */
char *read_str(FILE *file) {
	int i;
	char c;

	fgets( buffer, 1000, file);
	i=0;
	while( buffer[i] =='\n' || buffer[i]=='\r' || buffer[i]==' ') i++;
	do {
		c = buffer[ i++];
	} while( c != 0 && c != ' ' && c!='\n' && c!='\r'  && c!='#');
	buffer[ i -1] = 0;
	return buffer;
}

/**
 *
 * @param file
 * @return
 */
int read_int( FILE *file) {
	int i, ret;
	char c;

	fgets( buffer, 1000, file);
	i=0;
	while( buffer[i] =='\n' || buffer[i]=='\r' || buffer[i]==' ') i++;
	do {
		c = buffer[ i++];
	} while( c != 0 && c != ' '  && c!='\n' && c!='\r'  && c!='#');
	buffer[ i -1] = 0;
	sscanf( buffer, "%d", &ret);
	return ret;
}
#endif
/********************************************************************************************
*   TSS_DAA_SELECTED_ATTRIB
* this struct is used internally and externally, only a call to
internal_2_DAA_SELECTED_ATTRIB
* and DAA_SELECTED_ATTRIB_2_internal will change the struct to be internal or
external
********************************************************************************************/

void i_2_e_TSS_DAA_SELECTED_ATTRIB(
	TSS_DAA_SELECTED_ATTRIB *selected_attrib
) {
}

void e_2_i_TSS_DAA_SELECTED_ATTRIB(
	TSS_DAA_SELECTED_ATTRIB *selected_attrib
) {
}

/* work ONLY with internal format
important: TSS_BOOL is of type int_8_t, so a char, if the size is bigger, we
will maybe need
to transform each part to big indian ? or maybe each part is false if equal to
0, true otherwise.
*/
BYTE *to_bytes_TSS_DAA_SELECTED_ATTRIB_internal(
	int *result_length,
	TSS_DAA_SELECTED_ATTRIB *selected_attrib
) {
	BYTE *result;
	int index = 0;
	unsigned int length = selected_attrib->indicesListLength;

	*result_length = sizeof(unsigned int) +
				(selected_attrib->indicesListLength * sizeof(TSS_BOOL));
	result = (BYTE *)malloc( *result_length);
	memcpy( &result[index], &length, sizeof(UINT32));
	index+=sizeof(UINT32);
	memcpy( &result[index], selected_attrib->indicesList,
			sizeof(TSS_BOOL) * selected_attrib->indicesListLength);
	return result;
}

/*
create a TSS_DAA_SELECTED_ATTRIB of length <length> with given selected attributes.
example of selections of the second and third attributes upon 5:
create_TSS_DAA_SELECTED_ATTRIB( &selected_attrib, 5, 0, 1, 1, 0, 0);
*/
void create_TSS_DAA_SELECTED_ATTRIB( TSS_DAA_SELECTED_ATTRIB *attrib, int length, ...) {
	va_list ap;
	int i, select;

	attrib->indicesListLength = length;
	attrib->indicesList = (TSS_BOOL *)malloc( length * sizeof( TSS_BOOL));
	va_start (ap, length);
	for( i=0; i<length; i++) {
		select = va_arg( ap, int) != 0;
		attrib->indicesList[i] = select;
	}
	va_end (ap);
}


/******************************************************************************************
* TSS_DAA_SIGN_DATA
* this struct is used internally and externally, only a call to internal_2_DAA_SIGN_DATA
* DAA_SIGN_DATA_2_internal will change the struct to be internal or external
*******************************************************************************************/

void i_2_e_TSS_DAA_SIGN_DATA( TSS_DAA_SIGN_DATA *sign_data) {
}

void e_2_i_TSS_DAA_SIGN( TSS_DAA_SIGN_DATA *sign_data) {
}

/********************************************************************************************
*   TSS_DAA_ATTRIB_COMMIT
********************************************************************************************/

TSS_DAA_ATTRIB_COMMIT_internal *create_TSS_DAA_ATTRIB_COMMIT( bi_ptr beta, bi_ptr sMu) {
	TSS_DAA_ATTRIB_COMMIT_internal *result =
		(TSS_DAA_ATTRIB_COMMIT_internal *)malloc( sizeof(TSS_DAA_ATTRIB_COMMIT_internal));

	result->beta = beta;
	result->sMu = sMu;
	return result;
}


/********************************************************************************************
*  TSS_DAA_PSEUDONYM_PLAIN
********************************************************************************************/

TSS_DAA_PSEUDONYM_PLAIN_internal *
create_TSS_DAA_PSEUDONYM_PLAIN(bi_ptr nV)
{
	TSS_DAA_PSEUDONYM_PLAIN_internal *result = (TSS_DAA_PSEUDONYM_PLAIN_internal *)
		malloc(sizeof(TSS_DAA_PSEUDONYM_PLAIN_internal));

	result->nV = nV;
	return result;
}

/********************************************************************************************
*   DAA PRIVATE KEY
********************************************************************************************/
/*
* allocate: 	ret->p_prime
* 					ret->q_prime
* 				  	ret->productPQprime
*/
DAA_PRIVATE_KEY_internal *
create_TSS_DAA_PRIVATE_KEY(bi_ptr pPrime, bi_ptr qPrime)
{
	DAA_PRIVATE_KEY_internal *private_key =
		(DAA_PRIVATE_KEY_internal *)malloc( sizeof( DAA_PRIVATE_KEY_internal));

	private_key->p_prime = bi_new_ptr(); bi_set( private_key->p_prime, pPrime);
	private_key->q_prime = bi_new_ptr(); bi_set( private_key->q_prime, qPrime);
	private_key->productPQprime = bi_new_ptr();
	bi_mul( private_key->productPQprime, pPrime, qPrime);
	return private_key;
}

#if 0
int
save_DAA_PRIVATE_KEY(FILE *file, const DAA_PRIVATE_KEY_internal *private_key)
{
	BI_SAVE( private_key->p_prime	, file);
	BI_SAVE( private_key->q_prime	, file);
	BI_SAVE( private_key->productPQprime, file);
	return 0;
}

DAA_PRIVATE_KEY_internal *
load_DAA_PRIVATE_KEY(FILE *file)
{
	DAA_PRIVATE_KEY_internal *private_key =
		(DAA_PRIVATE_KEY_internal *)malloc( sizeof(DAA_PRIVATE_KEY_internal));

	private_key->p_prime = bi_new_ptr();
	BI_LOAD( private_key->p_prime, file);
	private_key->q_prime = bi_new_ptr();
	BI_LOAD( private_key->q_prime, file);
	private_key->productPQprime = bi_new_ptr();
	BI_LOAD( private_key->productPQprime, file);
	return private_key;
}
#endif
DAA_PRIVATE_KEY_internal *e_2_i_TSS_DAA_PRIVATE_KEY(TSS_DAA_PRIVATE_KEY *private_key) {
	DAA_PRIVATE_KEY_internal *private_key_internal;

	LogDebug("-> e_2_i_TSS_DAA_PRIVATE_KEY");
	private_key_internal =
		(DAA_PRIVATE_KEY_internal *)malloc( sizeof(DAA_PRIVATE_KEY_internal));
	private_key_internal->p_prime = get_bi( private_key->p_primeLength, private_key->p_prime);
	private_key_internal->q_prime = get_bi( private_key->q_primeLength, private_key->q_prime);
	private_key_internal->productPQprime =
		get_bi( private_key->productPQprimeLength, private_key->productPQprime);
	LogDebug("<- e_2_i_TSS_DAA_PRIVATE_KEY");
	return private_key_internal;
}

TSS_DAA_PRIVATE_KEY *
i_2_e_TSS_DAA_PRIVATE_KEY(DAA_PRIVATE_KEY_internal *private_key_internal,
			  void * (*daa_alloc)(size_t size, TSS_HOBJECT object),
			  TSS_HOBJECT param_alloc)
{
	TSS_DAA_PRIVATE_KEY *result;

	LogDebug("-> i_2_e_TSS_DAA_PRIVATE_KEY");
	result = (TSS_DAA_PRIVATE_KEY *)daa_alloc( sizeof(TSS_DAA_PRIVATE_KEY), param_alloc);
	init_tss_version( result);
	store_bi( &(result->p_primeLength),
			&(result->p_prime),
			private_key_internal->p_prime,
			daa_alloc,
			param_alloc);
	store_bi( &(result->q_primeLength),
			&(result->q_prime),
			private_key_internal->q_prime,
			daa_alloc,
			param_alloc);
	store_bi( &(result->productPQprimeLength),
			&(result->productPQprime),
			private_key_internal->productPQprime,
			daa_alloc,
			param_alloc);
	LogDebug("<- i_2_e_TSS_DAA_PRIVATE_KEY");
	return result;
}

/********************************************************************************************
*   KEY PAIR WITH PROOF
********************************************************************************************/

#if 0

/* moved to daa_debug.c */

int
save_KEY_PAIR_WITH_PROOF(FILE *file,
			 KEY_PAIR_WITH_PROOF_internal *key_pair_with_proof)
{
	save_DAA_PK_internal( file,  key_pair_with_proof->pk);
	save_DAA_PRIVATE_KEY( file, key_pair_with_proof->private_key);
	save_DAA_PK_PROOF_internal( file, key_pair_with_proof->proof);

	return 0;
}

KEY_PAIR_WITH_PROOF_internal *
load_KEY_PAIR_WITH_PROOF(FILE *file)
{
	KEY_PAIR_WITH_PROOF_internal *key_pair_with_proof =
		(KEY_PAIR_WITH_PROOF_internal *)malloc(sizeof(KEY_PAIR_WITH_PROOF_internal));

	key_pair_with_proof->pk = load_DAA_PK_internal(file);
	key_pair_with_proof->private_key = load_DAA_PRIVATE_KEY(file);
	key_pair_with_proof->proof = load_DAA_PK_PROOF_internal(file);

	return key_pair_with_proof;
}

#endif
/* allocated using instrumented daa_alloc */
TSS_DAA_KEY_PAIR *get_TSS_DAA_KEY_PAIR(KEY_PAIR_WITH_PROOF_internal *key_pair_with_proof,
				       void * (*daa_alloc)(size_t size, TSS_HOBJECT object),
				       TSS_HOBJECT param_alloc)
{
	TSS_DAA_KEY_PAIR *result;

	LogDebug("-> i_2_e_KEY_PAIR_WITH_PROOF");

	result = (TSS_DAA_KEY_PAIR *)daa_alloc(sizeof(TSS_DAA_KEY_PAIR), param_alloc);
	init_tss_version(result);
	result->private_key = i_2_e_TSS_DAA_PRIVATE_KEY(key_pair_with_proof->private_key,
							daa_alloc, param_alloc);
	result->public_key = i_2_e_TSS_DAA_PK( key_pair_with_proof->pk, daa_alloc, param_alloc);

	LogDebug("<- i_2_e_KEY_PAIR_WITH_PROOF");

	return result;
}


/********************************************************************************************
*   TSS_DAA_PK
********************************************************************************************/

/* pk_internal->capitalY must be alocated using ALLOC_BI_ARRAY() */
void
populate_capitalY(TSS_DAA_PK_internal *pk_internal)
{
	int i;

	bi_new_array(pk_internal->capitalY,
		     pk_internal->capitalRReceiver->length + pk_internal->capitalRIssuer->length);

	// CAPITAL Y ( capitalRReceiver )
	for (i = 0; i < pk_internal->capitalRReceiver->length; i++)
		pk_internal->capitalY->array[i] = pk_internal->capitalRReceiver->array[i];
	// CAPITAL Y ( capitalRIssuer)
	for (i = 0; i < pk_internal->capitalRIssuer->length; i++)
		pk_internal->capitalY->array[pk_internal->capitalRReceiver->length+i] =
			pk_internal->capitalRIssuer->array[i];
}

void
compute_capitalSprime(TSS_DAA_PK_internal *pk_internal)
{
	bi_t bi_tmp;

	bi_new(bi_tmp);
	pk_internal->capitalSprime = bi_new_ptr();
	bi_shift_left( bi_tmp, bi_1, DAA_PARAM_SIZE_SPLIT_EXPONENT);
	bi_mod_exp(pk_internal->capitalSprime, pk_internal->capitalS, bi_tmp, pk_internal->modulus);
	bi_free( bi_tmp);
}

/*
* create anf feel a TSS_DAA_PK_internal structures
* ! this function keep pointer on all parameters
*/
TSS_DAA_PK_internal *
create_DAA_PK(const bi_ptr modulus,
	      const bi_ptr capitalS,
	      const bi_ptr capitalZ,
	      const bi_ptr capitalR0,
	      const bi_ptr capitalR1,
	      const bi_ptr gamma,
	      const bi_ptr capitalGamma,
	      const bi_ptr rho,
	      const bi_array_ptr capitalRReceiver,
	      const bi_array_ptr capitalRIssuer,
	      const int issuerBaseNameLength,
	      BYTE * const issuerBaseName)
{
	TSS_DAA_PK_internal *pk_internal;

	LogDebug("-> create_DAA_PK");
	pk_internal = (TSS_DAA_PK_internal *)malloc(sizeof(TSS_DAA_PK_internal));
	pk_internal->modulus = modulus;
	pk_internal->capitalS = capitalS;
	pk_internal->capitalZ = capitalZ;
	pk_internal->capitalR0 = capitalR0;
	pk_internal->capitalR1 = capitalR1;
	pk_internal->gamma = gamma;
	pk_internal->capitalGamma = capitalGamma;
	pk_internal->rho = rho;
	pk_internal->capitalRReceiver = capitalRReceiver;
	pk_internal->capitalRIssuer = capitalRIssuer;
	pk_internal->capitalY = ALLOC_BI_ARRAY();
	populate_capitalY( pk_internal);
	pk_internal->issuerBaseNameLength = issuerBaseNameLength;
	pk_internal->issuerBaseName = issuerBaseName;
	compute_capitalSprime( pk_internal);

	LogDebug("<- create_DAA_PK");

	return pk_internal;
}

#if 0

/* moved to daa_debug.c */

int
save_DAA_PK_internal(FILE *file, const TSS_DAA_PK_internal *pk_internal)
{
	char *buffer;

	LogDebug("-> save_DAA_PK_internal");

	BI_SAVE( pk_internal->modulus, file);
	BI_SAVE( pk_internal->capitalS, file);
	BI_SAVE( pk_internal->capitalZ, file);
	BI_SAVE( pk_internal->capitalR0, file);
	BI_SAVE( pk_internal->capitalR1, file);
	BI_SAVE( pk_internal->gamma, file);
	BI_SAVE( pk_internal->capitalGamma, file);
	BI_SAVE( pk_internal->rho, file);
	BI_SAVE_ARRAY( pk_internal->capitalRReceiver, file);
	BI_SAVE_ARRAY( pk_internal->capitalRIssuer, file);
	fprintf( file, "%d\n", pk_internal->issuerBaseNameLength);
	buffer = (char *)malloc( pk_internal->issuerBaseNameLength + 1);
	memcpy( buffer, pk_internal->issuerBaseName, pk_internal->issuerBaseNameLength);
	buffer[ pk_internal->issuerBaseNameLength] = 0;
	fprintf( file, "%s\n", buffer);
	free( buffer);

	LogDebug("<- save_DAA_PK_internal");

	return 0;
}

TSS_DAA_PK_internal *
load_DAA_PK_internal(FILE *file)
{
	TSS_DAA_PK_internal *pk_internal =
		(TSS_DAA_PK_internal *)malloc(sizeof(TSS_DAA_PK_internal));
	char *read_buffer;

	pk_internal->modulus = bi_new_ptr();
	BI_LOAD( pk_internal->modulus, file);
	pk_internal->capitalS = bi_new_ptr();
	BI_LOAD( pk_internal->capitalS, file);
	pk_internal->capitalZ = bi_new_ptr();
	BI_LOAD( pk_internal->capitalZ, file);
	pk_internal->capitalR0 = bi_new_ptr();
	BI_LOAD( pk_internal->capitalR0, file);
	pk_internal->capitalR1 = bi_new_ptr();
	BI_LOAD( pk_internal->capitalR1, file);
	pk_internal->gamma = bi_new_ptr();
	BI_LOAD( pk_internal->gamma, file);
	pk_internal->capitalGamma = bi_new_ptr();
	BI_LOAD( pk_internal->capitalGamma, file);
	pk_internal->rho = bi_new_ptr();
	BI_LOAD( pk_internal->rho, file);
	pk_internal->capitalRReceiver = ALLOC_BI_ARRAY();
	BI_LOAD_ARRAY( pk_internal->capitalRReceiver, file);
	pk_internal->capitalRIssuer = ALLOC_BI_ARRAY();
	BI_LOAD_ARRAY( pk_internal->capitalRIssuer, file);
	pk_internal->capitalY = ALLOC_BI_ARRAY();
	populate_capitalY( pk_internal);
	pk_internal->issuerBaseNameLength = read_int( file);
	read_buffer = read_str( file);
	pk_internal->issuerBaseName = malloc( pk_internal->issuerBaseNameLength);
	memcpy( pk_internal->issuerBaseName, read_buffer, pk_internal->issuerBaseNameLength);
	compute_capitalSprime( pk_internal);
	return pk_internal;
}

#endif
void
dump_DAA_PK_internal(char *name, TSS_DAA_PK_internal *pk_internal)
{
	LogDebug("Dump TSS_DAA_PK_internal:%s\n", name);

	DUMP_BI( pk_internal->modulus);
	DUMP_BI( pk_internal->capitalS);
	DUMP_BI( pk_internal->capitalZ);
	DUMP_BI( pk_internal->capitalR0);
	DUMP_BI( pk_internal->capitalR1);
	DUMP_BI( pk_internal->gamma);
	DUMP_BI( pk_internal->capitalGamma);
	DUMP_BI( pk_internal->rho);
	DUMP_BI_ARRAY( pk_internal->capitalRReceiver);
	DUMP_BI_ARRAY( pk_internal->capitalRIssuer);

	LogDebug("issuerBaseName = %s\n",  pk_internal->issuerBaseName);
	LogDebug("End Dump TSS_DAA_PK_internal:%s\n", name);
}

/*
* Encode the DAA_PK like java.security.Key#getEncoded
*/
BYTE *
encoded_DAA_PK_internal(int *result_length, const TSS_DAA_PK_internal *pk)
{
	int length_issuer_base_name = pk->issuerBaseNameLength;
	int total_length = DAA_PARAM_TSS_VERSION_LENGTH +
		5 * ((DAA_PARAM_SIZE_RSA_MODULUS / 8)+ sizeof(int)) +
		2 * ((DAA_PARAM_SIZE_MODULUS_GAMMA / 8)+sizeof(int)) +
		1 * ((DAA_PARAM_SIZE_RHO / 8)+sizeof(int)) +
		pk->capitalY->length*(((DAA_PARAM_SIZE_RSA_MODULUS / 8)+sizeof(int)))
		+ length_issuer_base_name;
	BYTE *result = (BYTE *)malloc(total_length);
	int i, index = 0, length, big_indian_length;

	if (result == NULL)
		return NULL;

	LogDebug("total_length=%d", total_length);
	for (index = 0; index < DAA_PARAM_TSS_VERSION_LENGTH; index++)
		result[index] = DAA_PARAM_TSS_VERSION[index];
	// n, capitalS, capitalZ, capitalR0, capitalR1
	length = DAA_PARAM_SIZE_RSA_MODULUS / 8;
	big_indian_length = length;
	memcpy(&result[index], &big_indian_length, sizeof(int));
	index += sizeof(int);
	bi_2_byte_array( &result[index], length, pk->modulus);
	index += length;
	memcpy(&result[index], &big_indian_length, sizeof(int));
	index += sizeof(int);
	bi_2_byte_array( &result[index], length, pk->capitalS);
	index += length;
	memcpy(&result[index], &big_indian_length, sizeof(int));
	index += sizeof(int);
	bi_2_byte_array( &result[index], length, pk->capitalZ);
	index += length;
	memcpy(&result[index], &big_indian_length, sizeof(int));
	index += sizeof(int);
	bi_2_byte_array( &result[index], length, pk->capitalR0);
	index += length;
	memcpy(&result[index], &big_indian_length, sizeof(int));
	index += sizeof(int);
	bi_2_byte_array( &result[index], length, pk->capitalR1);
	index += length;
	// gamma, capitalGamma
	length = DAA_PARAM_SIZE_MODULUS_GAMMA / 8;
	big_indian_length = length;
	memcpy(&result[index], &big_indian_length, sizeof(int));
	index += sizeof(int);
	bi_2_byte_array( &result[index], length, pk->gamma);
	index += length;
	memcpy(&result[index], &big_indian_length, sizeof(int));
	index += sizeof(int);
	bi_2_byte_array( &result[index], length, pk->capitalGamma);
	index += length;
	// rho
	length = DAA_PARAM_SIZE_RHO / 8;
	big_indian_length = length;
	memcpy(&result[index], &big_indian_length, sizeof(int));
	index += sizeof(int);
	bi_2_byte_array( &result[index], length, pk->rho);
	index += length;
	// capitalY
	length = DAA_PARAM_SIZE_RSA_MODULUS / 8;
	big_indian_length = length;

	for( i=0; i<pk->capitalY->length; i++) {
		memcpy( &result[index], &big_indian_length, sizeof(int));
		index+=sizeof(int);
		bi_2_byte_array( &result[index], length, pk->capitalY->array[i]);
		index+=length;
	}
	// basename
	memcpy( &result[index], pk->issuerBaseName, length_issuer_base_name);
	index+=length_issuer_base_name;
	*result_length = index;

	LogDebug("return length=%d", index);

	return result;
}

/* create anf feel a TSS_DAA_PK structures */
TSS_DAA_PK *
i_2_e_TSS_DAA_PK(TSS_DAA_PK_internal *pk_internal,
		 void *(*daa_alloc)(size_t size, TSS_HOBJECT param_alloc),
		 TSS_HOBJECT param_alloc)
{
	int i;
	int capitalYLength;
	int capitalYLength2;
	TSS_DAA_PK *pk;

	LogDebug("-> i_2_e_TSS_DAA_PK");
	pk = (TSS_DAA_PK *)daa_alloc( sizeof(TSS_DAA_PK), param_alloc);
	init_tss_version( pk);
	if (pk == NULL) {
		LogError("Can not allocate the TSS_DAA_PK structure");
		return NULL;
	}
	STORE_DAA_PK_BI( modulus, daa_alloc, param_alloc);
	STORE_DAA_PK_BI( capitalS, daa_alloc, param_alloc);
	STORE_DAA_PK_BI( capitalZ, daa_alloc, param_alloc);
	STORE_DAA_PK_BI( capitalR0, daa_alloc, param_alloc);
	STORE_DAA_PK_BI( capitalR1, daa_alloc, param_alloc);
	STORE_DAA_PK_BI( gamma, daa_alloc, param_alloc);
	STORE_DAA_PK_BI( capitalGamma, daa_alloc, param_alloc);
	STORE_DAA_PK_BI( rho, daa_alloc, param_alloc);
	capitalYLength = pk_internal->capitalY->length;
	capitalYLength2 = bi_nbin_size( pk_internal->capitalY->array[0]);
	LogDebug("[capitalYLength=%d capitalYLength2=%d total size=%d]\n",
		 capitalYLength, capitalYLength2, sizeof(BYTE) * capitalYLength *
				 capitalYLength2);
	pk->capitalY = (BYTE **) daa_alloc( sizeof(BYTE *) * capitalYLength, param_alloc );
	for (i = 0; i < capitalYLength; i++) {
		if( bi_nbin_size( pk_internal->capitalY->array[i]) != capitalYLength2) {
			// LOG ERROR
			LogError("Error during feel operation of capitalY (index=%d capitalYLength"
				 "2=%d, currentSize=%d)\n", i, capitalYLength2,
				(int)bi_nbin_size(pk_internal->capitalY->array[i]));
		}
		BYTE *buffer = (BYTE*) daa_alloc( sizeof(BYTE) * capitalYLength2, param_alloc);
		bi_2_byte_array( buffer, capitalYLength2, pk_internal->capitalY->array[i]);
		// bi_2_nbin1( &checkSize, buffer,	  pk_internal->capitalY->array[i]);
		pk->capitalY[i] = buffer;
		LogDebug( "[i=%d currentsize=%d  buffer[%d]=[%2x|%2x]\n",
			i,
			(int)bi_nbin_size( pk_internal->capitalY->array[i]),
			(int)pk->capitalY[i],
			(int)pk->capitalY[i][0],
			(int)pk->capitalY[i][1]);
	}
	pk->capitalYLength = capitalYLength;
	pk->capitalYLength2 = capitalYLength2;
	pk->capitalYPlatformLength = pk_internal->capitalRReceiver->length;
	LogDebug("issuer= len=%d", pk_internal->issuerBaseNameLength);
	pk->issuerBaseNameLength = pk_internal->issuerBaseNameLength;
	pk->issuerBaseName = (BYTE *)daa_alloc(pk_internal->issuerBaseNameLength, param_alloc);
	memcpy( pk->issuerBaseName,
			pk_internal->issuerBaseName,
			pk_internal->issuerBaseNameLength);
	LogDebug("i_2_e_TSS_DAA_PK extern_issuer=%s   intern_issuer=%s\n",
			pk->issuerBaseName,
			pk_internal->issuerBaseName);
	LogDebug("<- i_2_e_TSS_DAA_PK");
	return pk;
}

/**/
TSS_DAA_PK_internal *
e_2_i_TSS_DAA_PK( TSS_DAA_PK *pk)
{
	TSS_DAA_PK_internal *pk_internal =
		(TSS_DAA_PK_internal *)malloc(sizeof(TSS_DAA_PK_internal));
	unsigned long capitalYLength, capitalYLength2, capitalYPlatformLength;
	UINT32 i;
	int issuer_length;

	// pk_internal->modulus = GET_DAA_PK_BI( modulus);
	pk_internal->modulus = get_bi(pk->modulusLength, pk->modulus);
	pk_internal->capitalS = get_bi(pk->capitalSLength, pk->capitalS);
	pk_internal->capitalZ = get_bi(pk->capitalZLength, pk->capitalZ);
	pk_internal->capitalR0 = get_bi(pk->capitalR0Length, pk->capitalR0);
	pk_internal->capitalR1 = get_bi(pk->capitalR1Length, pk->capitalR1);
	pk_internal->gamma = get_bi(pk->gammaLength, pk->gamma);
	pk_internal->capitalGamma = get_bi(pk->capitalGammaLength, pk->capitalGamma);
	pk_internal->rho = get_bi(pk->rhoLength, pk->rho);
	capitalYLength = pk->capitalYLength;
	capitalYLength2= pk->capitalYLength2;
	capitalYPlatformLength = pk->capitalYPlatformLength;
	LogDebug( "capitalYLength:%ld  capitalYLength2:%ld capitalYPlatformLength:%ld\n",
		capitalYLength, capitalYLength2, capitalYPlatformLength);

	pk_internal->capitalRReceiver = ALLOC_BI_ARRAY();
	bi_new_array2(pk_internal->capitalRReceiver, capitalYPlatformLength);
	for (i = 0; i < capitalYPlatformLength; i++) {
		LogDebug( "i=%d\n", i);
		pk_internal->capitalRReceiver->array[i] =
			get_bi(pk->capitalYLength2, pk->capitalY[i]);
	}
	pk_internal->capitalRIssuer = ALLOC_BI_ARRAY();
	bi_new_array2( pk_internal->capitalRIssuer, capitalYLength -
			capitalYPlatformLength);
	for( ; i<capitalYLength; i++) {
		pk_internal->capitalRIssuer->array[ i - capitalYPlatformLength] =
			get_bi( pk->capitalYLength2, pk->capitalY[i]);
	}
	pk_internal->capitalY = ALLOC_BI_ARRAY();
	populate_capitalY( pk_internal);
	issuer_length = pk->issuerBaseNameLength;
	pk_internal->issuerBaseNameLength = issuer_length;
	LogDebug( "issuer_length=%d\n", issuer_length);
	pk_internal->issuerBaseName = (BYTE *)malloc( issuer_length);
	memcpy( pk_internal->issuerBaseName, pk->issuerBaseName, issuer_length);
	LogDebug("e_2_i_TSS_DAA_PK extern_issuer=%s   intern_issuer=%s\n",
		pk->issuerBaseName,
		pk_internal->issuerBaseName);
	compute_capitalSprime( pk_internal); // allocation
	return pk_internal;
}

void
free_TSS_DAA_PK_internal(TSS_DAA_PK_internal *pk_internal)
{
	bi_free_ptr( pk_internal->capitalSprime);
	free( pk_internal->issuerBaseName);
	free( pk_internal->capitalY);
	bi_free_array( pk_internal->capitalRIssuer);
	bi_free_array( pk_internal->capitalRReceiver);
	bi_free_ptr( pk_internal->rho);
	bi_free_ptr( pk_internal->capitalGamma);
	bi_free_ptr( pk_internal->gamma);
	bi_free_ptr( pk_internal->capitalR1);
	bi_free_ptr( pk_internal->capitalR0);
	bi_free_ptr( pk_internal->capitalZ);
	bi_free_ptr( pk_internal->capitalS);
	bi_free_ptr( pk_internal->modulus);
	free( pk_internal);
}

/* free a TSS_DAA_PK structures */
void
free_TSS_DAA_PK(TSS_DAA_PK *pk)
{
	int i;

	LogDebug("-> free_TSS_DAA_PK");
	free( pk->modulus);
	free( pk->capitalS);
	free( pk->capitalZ);
	free( pk->capitalR0);
	free( pk->capitalR1);
	free( pk->gamma);
	free( pk->capitalGamma);
	free( pk->rho);
	for(  i=0; i<(int)pk->capitalYLength; i++) {
		free( pk->capitalY[i]);
	}
	free( pk->capitalY);
	free( pk->issuerBaseName);
	free( pk);
	LogDebug("<- free_TSS_DAA_PK");

}

TPM_DAA_ISSUER *
convert2issuer_settings(TSS_DAA_PK_internal *pk_internal)
{
	TPM_DAA_ISSUER *result = (TPM_DAA_ISSUER *)malloc(sizeof(TPM_DAA_ISSUER));
	EVP_MD_CTX mdctx;
	UINT32 length;
	BYTE *array = (BYTE*)malloc((DAA_PARAM_SIZE_RSA_MODULUS+7)/8);

	LogDebug("convert2issuer_settings");
	EVP_MD_CTX_init(&mdctx);
	// TAG
	result->tag = htons( TPM_TAG_DAA_ISSUER);
	// capitalR0
	EVP_DigestInit(&mdctx, DAA_PARAM_get_message_digest());

	EVP_DigestInit_ex(&mdctx, DAA_PARAM_get_message_digest(), NULL);
	bi_2_byte_array( array,
			length = (bi_length( pk_internal->capitalR0)+7)/8,
			pk_internal->capitalR0);
	LogDebug("capitalR0 length=%d", length);
	EVP_DigestUpdate(&mdctx, array, length);
	EVP_DigestFinal_ex(&mdctx, (BYTE *)&(result->DAA_digest_R0), NULL);
	// capitalR1
	EVP_DigestInit_ex(&mdctx, DAA_PARAM_get_message_digest(), NULL);
	bi_2_byte_array( array,
			length = (bi_length( pk_internal->capitalR1)+7)/8,
			pk_internal->capitalR1);
	LogDebug("capitalR1 length=%d", length);
	EVP_DigestUpdate(&mdctx, array, length);
	EVP_DigestFinal_ex(&mdctx, (BYTE *)&(result->DAA_digest_R1), NULL);
	// capitalS (S0)
	EVP_DigestInit_ex(&mdctx, DAA_PARAM_get_message_digest(), NULL);
	bi_2_byte_array( array,
			length = (bi_length( pk_internal->capitalS)+7)/8,
			pk_internal->capitalS);
	LogDebug("capitalS length=%d", length);
	EVP_DigestUpdate(&mdctx, array, length);
	EVP_DigestFinal_ex(&mdctx, (BYTE *)&(result->DAA_digest_S0), NULL);
	// capitalSprime (S1)
	EVP_DigestInit_ex(&mdctx, DAA_PARAM_get_message_digest(), NULL);
	bi_2_byte_array( array,
			length = (bi_length( pk_internal->capitalSprime)+7)/8,
			pk_internal->capitalSprime);
	LogDebug("capitalSprime length=%d", length);
	EVP_DigestUpdate(&mdctx, array, length);
	EVP_DigestFinal_ex(&mdctx, (BYTE *)&(result->DAA_digest_S1), NULL);
	// modulus (n)
	EVP_DigestInit_ex(&mdctx, DAA_PARAM_get_message_digest(), NULL);
	bi_2_byte_array( array,
			length = (bi_length( pk_internal->modulus)+7)/8,
			pk_internal->modulus);
	LogDebug("modulus length=%d", length);
	EVP_DigestUpdate(&mdctx, array, length);
	EVP_DigestFinal_ex(&mdctx, (BYTE *)&(result->DAA_digest_n), NULL);
	// modulus (n)
	EVP_DigestInit_ex(&mdctx, DAA_PARAM_get_message_digest(), NULL);
	bi_2_byte_array( array,
			length = (bi_length( pk_internal->capitalGamma)+7)/8,
			pk_internal->capitalGamma);
	LogDebug("capitalGamma length=%d", length);
	EVP_DigestUpdate(&mdctx, array, length);
	free(array);
	EVP_DigestFinal_ex(&mdctx, (BYTE *)&(result->DAA_digest_gamma), NULL);
	EVP_MD_CTX_cleanup(&mdctx);
	// rho
	bi_2_byte_array( (BYTE *)&(result->DAA_generic_q), 26, pk_internal->rho);
	return result;
}

BYTE *
issuer_2_byte_array(TPM_DAA_ISSUER *tpm_daa_issuer, int *length)
{
	UINT32 size = sizeof(UINT16) + ( 6 * TPM_SHA1_160_HASH_LEN) + 26;
	BYTE * result = (BYTE *)malloc( sizeof(BYTE)*size);
	UINT32 i = 0;

	memcpy( &result[i], &(tpm_daa_issuer->tag), sizeof(UINT16));
	i+=sizeof(UINT16);
	memcpy( &result[i], &(tpm_daa_issuer->DAA_digest_R0), TPM_SHA1_160_HASH_LEN);
	i+=TPM_SHA1_160_HASH_LEN;
	memcpy( &result[i], &(tpm_daa_issuer->DAA_digest_R1), TPM_SHA1_160_HASH_LEN);
	i+=TPM_SHA1_160_HASH_LEN;
	memcpy( &result[i], &(tpm_daa_issuer->DAA_digest_S0), TPM_SHA1_160_HASH_LEN);
	i+=TPM_SHA1_160_HASH_LEN;
	memcpy( &result[i], &(tpm_daa_issuer->DAA_digest_S1), TPM_SHA1_160_HASH_LEN);
	i+=TPM_SHA1_160_HASH_LEN;
	memcpy( &result[i], &(tpm_daa_issuer->DAA_digest_n),   TPM_SHA1_160_HASH_LEN);
	i+=TPM_SHA1_160_HASH_LEN;
	memcpy( &result[i], &(tpm_daa_issuer->DAA_digest_gamma), TPM_SHA1_160_HASH_LEN);
	i+=TPM_SHA1_160_HASH_LEN;
	memcpy( &result[i], &(tpm_daa_issuer->DAA_generic_q), 26);
	*length = size;
	return result;
}

/********************************************************************************************
*   TSS_DAA_PK_PROOF
********************************************************************************************/

/*
* this function keep references on:
* - challenge (BYTE *)
* - response (bi_array_ptr *)
*/
TSS_DAA_PK_PROOF_internal *
create_DAA_PK_PROOF(BYTE* const challenge,
		    const int length_challenge,
		    bi_array_ptr *response,
		    const int length_response)
{
	TSS_DAA_PK_PROOF_internal *pk_proof;

#ifdef DAA_DEBUG
	printf("create_DAA_PK_PROOF_internal\n");
#endif
	pk_proof = (TSS_DAA_PK_PROOF_internal *)malloc( sizeof(TSS_DAA_PK_PROOF_internal));
	pk_proof->challenge = challenge;
	pk_proof->length_challenge = length_challenge;
	pk_proof->response = response;
	pk_proof->length_response = length_response;
	return pk_proof;
}

#if 0
int
save_DAA_PK_PROOF_internal(FILE *file, TSS_DAA_PK_PROOF_internal *proof)
{
	int i;

#ifdef DAA_DEBUG
	printf("save_DAA_PK_PROOF_internal");
#endif
	fprintf(file, "%d # %s.length\n", proof->length_challenge, "challenge");
	fprintf(file, "%s\n", dump_byte_array( proof->length_challenge,
		proof->challenge));
	fprintf(file, "%d # %s.length\n", proof->length_response, "response");
	for (i = 0; i < proof->length_response; i++) {
		BI_SAVE_ARRAY( proof->response[i], file);
	}

	return 0;
}

/* load <proof> using <filename> */
/* allocation of: */
/* 		proof->challenge (BYTE*) */
/*		response (bi_array_ptr) */
TSS_DAA_PK_PROOF_internal *
load_DAA_PK_PROOF_internal(FILE *file)
{
	TSS_DAA_PK_PROOF_internal *proof =
		(TSS_DAA_PK_PROOF_internal *)malloc(sizeof(TSS_DAA_PK_PROOF_internal));
	char *read_buffer;
	int  i;

#ifdef DAA_DEBUG
	printf("load_DAA_PK_PROOF_internal");
#endif
	proof->length_challenge = read_int( file);
	read_buffer = read_str( file);
	proof->challenge = retrieve_byte_array( &(proof->length_challenge),read_buffer);
	proof->length_response = read_int( file);
	proof->response = (bi_array_ptr *)malloc( sizeof(bi_array_ptr) * proof->length_response);
	for (i = 0; i < proof->length_response; i++) {
		proof->response[i] = ALLOC_BI_ARRAY();
		BI_LOAD_ARRAY( proof->response[i], file);
	}
	return proof;
}
#endif

TSS_DAA_PK_PROOF *
i_2_e_TSS_DAA_PK_PROOF(TSS_DAA_PK_PROOF_internal*pk_internal_proof,
		       void * (*daa_alloc)(size_t size, TSS_HOBJECT param),
		       TSS_HOBJECT param_alloc)
{
	TSS_DAA_PK_PROOF *pk_proof =
		(TSS_DAA_PK_PROOF *)daa_alloc( sizeof(TSS_DAA_PK_PROOF), param_alloc);
	int i, j;
	int length_response2;
	int length_response3;

	init_tss_version( pk_proof);
	// CHALLENGE
	pk_proof->challengeLength = pk_internal_proof->length_challenge;
	pk_proof->challenge = (BYTE *)daa_alloc( pk_internal_proof->length_challenge,
						param_alloc);
	memcpy( pk_proof->challenge, pk_internal_proof->challenge,
		pk_internal_proof->length_challenge);
	// RESPONSES
	pk_proof->responseLength = pk_internal_proof->length_response;
	length_response2 = pk_internal_proof->response[0]->length;
	pk_proof->responseLength2 = length_response2;
	length_response3 = bi_nbin_size(
			pk_internal_proof->response[0]->array[0]);
	if( length_response3 & 1) length_response3++; // length_response3 should be paire
			pk_proof->responseLength3 = length_response3;
	pk_proof->response = (BYTE ***)daa_alloc( sizeof(BYTE **) *
			pk_internal_proof->length_response, param_alloc);
	for(i = 0; i < pk_internal_proof->length_response; i++) {
		pk_proof->response[i] = (BYTE **)daa_alloc( sizeof(BYTE *) * length_response2,
							  param_alloc);
		for( j = 0; j < length_response2; j++) {
			(pk_proof->response[i])[j] = (BYTE *)malloc(
					sizeof(BYTE) * length_response3);
			bi_2_byte_array( pk_proof->response[i][j],
					length_response3,
					pk_internal_proof->response[i]->array[j]);
		}
	}
	return pk_proof;
}

TSS_DAA_PK_PROOF_internal *
e_2_i_TSS_DAA_PK_PROOF(TSS_DAA_PK_PROOF *pk_proof)
{
	int i, j, response_length2;
	TSS_DAA_PK_PROOF_internal *pk_proof_internal =
			(TSS_DAA_PK_PROOF_internal *)malloc( sizeof( TSS_DAA_PK_PROOF_internal));

	// CHALLENGE
	pk_proof_internal->length_challenge = pk_proof->challengeLength;
#ifdef DAA_DEBUG
	fprintf(stderr, "issuer_length=%d\n", pk_proof_internal->length_challenge);
#endif
	pk_proof_internal->challenge = (BYTE *)malloc( pk_proof_internal->length_challenge);
	memcpy( pk_proof_internal->challenge,
		pk_proof->challenge,
		pk_proof_internal->length_challenge);
	// RESPONSES
	pk_proof_internal->length_response = pk_proof->responseLength;
	response_length2 = pk_proof->responseLength2;
	pk_proof_internal->response =
		(bi_array_ptr *)malloc( sizeof(bi_array_ptr) *
					pk_proof_internal->length_response);
	for(i = 0; i<pk_proof_internal->length_response; i++) {
		pk_proof_internal->response[i] = ALLOC_BI_ARRAY();
		bi_new_array2( pk_proof_internal->response[i], response_length2);
		for( j = 0; j < response_length2; j++) {
			pk_proof_internal->response[i]->array[j] =
				get_bi( pk_proof->responseLength3, pk_proof->response[i][j]);
		}
	}
	return pk_proof_internal;
}


/********************************************************************************************
*   TSS_DAA_JOIN_ISSUER_SESSION
********************************************************************************************/

TSS_DAA_JOIN_ISSUER_SESSION_internal *
create(TSS_DAA_PK_PROOF_internal *issuerKeyPair,
       TPM_DAA_ISSUER *issuerAuthKey,
       TSS_DAA_IDENTITY_PROOF *identityProof,
       bi_ptr capitalUprime,
       int daaCounter,
       int nonceIssuerLength,
       BYTE *nonceIssuer,
       int nonceEncryptedLength,
       BYTE *nonceEncrypted)
{
	TSS_DAA_JOIN_ISSUER_SESSION_internal *result =
		(TSS_DAA_JOIN_ISSUER_SESSION_internal *)malloc(
						sizeof(TSS_DAA_JOIN_ISSUER_SESSION_internal));

	result->issuerAuthKey = issuerAuthKey;
	result->issuerKeyPair = issuerKeyPair;
	result->identityProof = identityProof;
	result->capitalUprime = capitalUprime;
	result->daaCounter = daaCounter;
	result->nonceIssuerLength = nonceIssuerLength;
	result->nonceIssuer = nonceIssuer;
	result->nonceEncryptedLength = nonceEncryptedLength;
	result->nonceEncrypted = nonceEncrypted;
	return result;
}


/********************************************************************************************
 *   TSS_DAA_SIGNATURE
 ********************************************************************************************/

TSS_DAA_SIGNATURE_internal*
e_2_i_TSS_DAA_SIGNATURE(TSS_DAA_SIGNATURE* signature)
{
	TSS_DAA_SIGNATURE_internal *signature_intern =
		(TSS_DAA_SIGNATURE_internal *)malloc( sizeof( TSS_DAA_SIGNATURE_internal));
	int i, length;

	signature_intern->zeta = bi_set_as_nbin( signature->zetaLength, signature->zeta);
	signature_intern->capitalT = bi_set_as_nbin( signature->capitalTLength,
							signature->capitalT);
	signature_intern->challenge_length = signature->challengeLength;
	signature_intern->challenge = (BYTE *)malloc( signature->challengeLength);
	memcpy( signature_intern->challenge,
		signature->challenge,
		signature->challengeLength);
	signature_intern->nonce_tpm_length = signature->nonceTpmLength;
	signature_intern->nonce_tpm = (BYTE *)malloc( signature->nonceTpmLength);
	memcpy( signature_intern->nonce_tpm, signature->nonceTpm, signature->nonceTpmLength);
	signature_intern->sV = bi_set_as_nbin( signature->sVLength, signature->sV);
	signature_intern->sF0 = bi_set_as_nbin( signature->sF0Length, signature->sF0);
	signature_intern->sF1 = bi_set_as_nbin( signature->sF1Length, signature->sF1);
	signature_intern->sE = bi_set_as_nbin( signature->sELength, signature->sE);
	signature_intern->sA = (bi_array_ptr)malloc( sizeof( bi_array));
	bi_new_array2( signature_intern->sA, signature->sALength);
	length = ( DAA_PARAM_SIZE_RANDOMIZED_ATTRIBUTES + 7) / 8;
	for (i = 0; i < (int)signature->sALength; i++) {
		signature_intern->sA->array[i] = bi_set_as_nbin( length, signature->sA[i]);
	}

	return signature_intern;
}

void
free_TSS_DAA_SIGNATURE_internal(TSS_DAA_SIGNATURE_internal *signature)
{
	bi_free_array( signature->sA);
	bi_free_ptr( signature->sE);
	bi_free_ptr( signature->sF1);
	bi_free_ptr( signature->sF0);
	bi_free_ptr( signature->sV);
	free( signature->nonce_tpm);
	free( signature->challenge);
	bi_free_ptr( signature->capitalT);
	bi_free_ptr( signature->zeta);
	free( signature);
}

#if 0
/********************************************************************************************
	TSS_DAA_CRED_ISSUER
********************************************************************************************/

TSS_DAA_CRED_ISSUER *
load_TSS_DAA_CRED_ISSUER(FILE *file)
{
	TSS_DAA_CRED_ISSUER *credential =
		(TSS_DAA_CRED_ISSUER *)malloc(sizeof(TSS_DAA_CRED_ISSUER));
	char *read_buffer;
	int  i, len;

	init_tss_version( credential);
	credential->capitalALength = read_int( file);
	read_buffer = read_str( file);
	credential->capitalA = retrieve_byte_array( &(credential->capitalALength),
						read_buffer);
	credential->eLength = read_int( file);
	read_buffer = read_str( file);
	credential->e = retrieve_byte_array( &(credential->eLength),read_buffer);
	credential->vPrimePrimeLength = read_int( file);
	read_buffer = read_str( file);
	credential->vPrimePrime = retrieve_byte_array(&(credential->vPrimePrimeLength),
							read_buffer);
	// attributes issuer
	credential->attributesIssuerLength = read_int( file);
	credential->attributesIssuer = malloc(credential->attributesIssuerLength*sizeof(BYTE*));
	for( i=0; i < (int)credential->attributesIssuerLength; i++) {
		credential->attributesIssuer[i] = retrieve_byte_array( &len, read_buffer);
	}
	credential->cPrimeLength = read_int( file);
	read_buffer = read_str( file);
	credential->cPrime = retrieve_byte_array( &(credential->cPrimeLength),read_buffer);
	credential->sELength = read_int( file);
	read_buffer = read_str( file);
	credential->sE = retrieve_byte_array( &(credential->sELength),read_buffer);
	return credential;
}

int
save_TSS_DAA_CRED_ISSUER(FILE *file, TSS_DAA_CRED_ISSUER *credential)
{
	int i;

	fprintf(file, "%d # %s.length\n", credential->capitalALength, "capitalA");
	fprintf(file, "%s\n", dump_byte_array( credential->capitalALength,
						credential->capitalA));
	fprintf(file, "%d # %s.length\n", credential->eLength, "e");
	fprintf(file, "%s\n", dump_byte_array( credential->eLength,
						credential->e));
	fprintf(file, "%d # %s.length\n", credential->vPrimePrimeLength, "vPrimePrime");
	fprintf(file, "%s\n", dump_byte_array( credential->vPrimePrimeLength,
						credential->vPrimePrime));
	fprintf(file, "%d # %s\n", credential->attributesIssuerLength, "attributesIssuerLength");
	for( i=0; i < (int)credential->attributesIssuerLength; i++) {
		fprintf(file, "%s\n", dump_byte_array( DAA_PARAM_SIZE_F_I / 8,
							credential->attributesIssuer[i]));

	}
	fprintf(file, "%d # %s.length\n", credential->cPrimeLength, "cPrime");
	fprintf(file, "%s\n", dump_byte_array( credential->cPrimeLength,
						credential->cPrime));
	fprintf(file, "%d # %s.length\n", credential->sELength, "sE");
	fprintf(file, "%s\n", dump_byte_array( credential->sELength,
						credential->sE));
	return 0;
}


/********************************************************************************************
	TSS_DAA_CREDENTIAL
********************************************************************************************/

TSS_DAA_CREDENTIAL *
load_TSS_DAA_CREDENTIAL(FILE *file)
{
	TSS_DAA_CREDENTIAL *credential =
		(TSS_DAA_CREDENTIAL *)malloc(sizeof(TSS_DAA_CREDENTIAL));
	char *read_buffer;
	int  i, len;
	TSS_DAA_PK_internal *pk_internal;
	TSS_DAA_PK *pk;

	init_tss_version( credential);
	credential->capitalALength = read_int( file);
	read_buffer = read_str( file);
	credential->capitalA = retrieve_byte_array( &(credential->capitalALength),
						read_buffer);
	credential->exponentLength = read_int( file);
	read_buffer = read_str( file);
	credential->exponent = retrieve_byte_array( &(credential->exponentLength),
						read_buffer);
	credential->vBar0Length = read_int( file);
	read_buffer = read_str( file);
	credential->vBar0 = retrieve_byte_array(&(credential->vBar0Length),
						read_buffer);
	credential->vBar1Length = read_int( file);
	read_buffer = read_str( file);
	credential->vBar1 = retrieve_byte_array(&(credential->vBar1Length),
						read_buffer);
	// attributes issuer
	credential->attributesLength = read_int( file);
	printf("attributesLength=%d\n", credential->attributesLength);
	credential->attributes = malloc(credential->attributesLength * sizeof( BYTE *));
	for( i=0; i < (int)credential->attributesLength; i++) {
		read_buffer = read_str( file);
		credential->attributes[i] = retrieve_byte_array( &len, read_buffer);
		if( len != DAA_PARAM_SIZE_F_I / 8) {
			LogError("Error when parsing attributes");
			LogError("\tattribute length:%d", len);
			LogError("\texpected length:%d", DAA_PARAM_SIZE_F_I / 8);
			return NULL;
		}
	}
	pk_internal = load_DAA_PK_internal( file);
	pk = i_2_e_TSS_DAA_PK( pk_internal, &normal_malloc, (TSS_HOBJECT)NULL);
	memcpy( &(credential->issuerPK), pk, sizeof(TSS_DAA_PK));
	free( pk);
	free_TSS_DAA_PK_internal( pk_internal);
	credential->tpmSpecificEncLength = read_int( file);
	read_buffer = read_str( file);
	credential->tpmSpecificEnc = retrieve_byte_array( &(credential->tpmSpecificEncLength),
							read_buffer);
	credential->daaCounter = read_int( file);
	return credential;
}

int
save_TSS_DAA_CREDENTIAL(FILE *file,
			TSS_DAA_CREDENTIAL *credential)
{
	int i;
	TSS_DAA_PK_internal *pk_internal;

	fprintf(file, "%d # %s.length\n", credential->capitalALength, "capitalA");
	fprintf(file, "%s\n", dump_byte_array( credential->capitalALength,
						credential->capitalA));
	fprintf(file, "%d # %s.length\n", credential->exponentLength, "exponent");
	fprintf(file, "%s\n", dump_byte_array( credential->exponentLength,
						credential->exponent));
	fprintf(file, "%d # %s.length\n", credential->vBar0Length, "vBar0");
	fprintf(file, "%s\n", dump_byte_array( credential->vBar0Length,
						credential->vBar0));
	fprintf(file, "%d # %s.length\n", credential->vBar1Length, "vBar1");
	fprintf(file, "%s\n", dump_byte_array( credential->vBar1Length,
						credential->vBar1));
	fprintf(file, "%d # %s\n", credential->attributesLength, "attributesLength");
	for( i=0; i < (int)credential->attributesLength; i++) {
		fprintf(file, "%s\n", dump_byte_array( DAA_PARAM_SIZE_F_I / 8,
							credential->attributes[i]));
	}
	pk_internal = e_2_i_TSS_DAA_PK( &(credential->issuerPK) );
	save_DAA_PK_internal( file, pk_internal);
	free_TSS_DAA_PK_internal( pk_internal);
	fprintf(file, "%d # %s.length\n", credential->tpmSpecificEncLength, "tpmSpecificEnc");
	fprintf(file, "%s\n", dump_byte_array( credential->tpmSpecificEncLength,
						credential->tpmSpecificEnc));
	fprintf(file, "%d # daaCounter\n", credential->daaCounter);
	return 0;
}
#endif
/********************************************************************************************
	TPM_DAA_ISSUER
********************************************************************************************/

void
free_TPM_DAA_ISSUER(TPM_DAA_ISSUER *tpm_daa_issuer)
{
	free(tpm_daa_issuer);
}
