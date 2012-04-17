
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#include <bi.h>

#include "tcslog.h"

#undef INLINE_DECL
#define INLINE_DECL

/***********************************************************************************
	CONSTANT
*************************************************************************************/

bi_t bi_0;
bi_t bi_1;
bi_t bi_2;

/***********************************************************************************
	WORK VARIABLE
*************************************************************************************/

// Buffer to load bi from a file . A static field is used, this should not be
// refere in any SPI calls.
#define BUFFER_SIZE 10000
static char buffer[BUFFER_SIZE]; // used for loading bi

#define SAFETY_PARAM 80

// keep the list of allocated memory, usually used for the format functions
list_ptr allocs = NULL;

/***********************************************************************************
	DUMP LIB
*************************************************************************************/

// !! to use only for debugging
// do not used it in the same call, as a static buffer is used
char *dump_byte_array(int len, unsigned char *array) {
	int i, j=0;
	char c, str[3];

	for( i=0; i<len; i++) {
		c = array[i];
		sprintf( str, "%02X", (int)(c & 0xFF));
		buffer[j] = str[0];
		buffer[j+1] = str[1];
		j+=2;
	}
	buffer[j] = 0;
	return buffer;
}

/* convert <strings> and return it into a byte array <result> of length <length> */
unsigned char *retrieve_byte_array( int *len, const char *strings) {
	int index_str = 0, index_result = 0;
	int str_len = strlen( strings);
	char read_buffer[3];
	int c;
	unsigned char *result;

	read_buffer[2]=0;
	*len = ( str_len >> 1);
	#ifdef BI_DEBUG
	printf("[%s]\n", strings);
	printf("[retrieve_byte_array] strlen=%d len=%d\n", str_len, *len);
	#endif
	result = (unsigned char *)malloc( *len+1);
	if( result == NULL) {
		LogError("malloc of %d bytes failed", *len+1);
		return NULL;
	}
	if( (str_len & 1) ==1) {
		// impair =>   1 12 23 -> 01 12 23
		read_buffer[0]='0';
		read_buffer[1]=strings[index_str++];
		sscanf( read_buffer, "%2X", &c);
		#ifdef BI_DEBUG
		printf("[c'=%2X|%s]", (int)(c & 0xFF), read_buffer);
		#endif
		result[index_result++] = c&0xFF;
		(*len)++;
	}
	while( index_str < str_len) {
		read_buffer[0] = strings[ index_str++];
		read_buffer[1] = strings[ index_str++];
		sscanf( read_buffer, "%02X", &c);
		#ifdef BI_DEBUG
		printf("[c'=%2X|%s]", (int)(c & 0xFF), read_buffer);
		#endif
		result[index_result++] = c&0xFF;
	}
	return result;
}

/* create a <big integer> array */
INLINE_DECL void bi_new_array( bi_array array, const int length) {
	int i=0;

	bi_new_array2( array, length);
	if( array->array == NULL) return;
	for( i = 0; i< length; i++) {
		array->array[i] = bi_new_ptr();
	}
}

/* create a <big integer> array */
INLINE_DECL void bi_new_array2( bi_array array, const int length) {
	array->length = length;
	array->array = (bi_ptr *)malloc( sizeof(bi_ptr) * length);
	if( array->array == NULL) {
		LogError("malloc of %d bytes failed", sizeof(bi_ptr)*length);
		return;
	}
}

/* free resources allocated to the big integer <i> */
INLINE_DECL void bi_free_array(bi_array array) {
	int length = array->length;
	int i=0;

	for( i = 0; i< length; i++) {
		bi_free_ptr( array->array[i]);
	}
	free( array->array);
}

/* copy length pointers from the array <src, offset_src> to array <dest, offset_dest> */
INLINE_DECL void bi_copy_array(bi_array_ptr src,
				int offset_src,
				bi_array_ptr dest,
				int offset_dest,
				int length) {
	int i=0;

	for( i = 0; i< length; i++) {
		dest->array[ offset_dest + i] = src->array[ offset_src + i];
	}
}

/* debug function -> dump a field of type bi_array */
void dump_bi_array( char *field, const bi_array_ptr array) {
	int i;

	for( i=0; i<array->length; i++) {
		printf("%s->array[%d] = %s\n", field, i, bi_2_hex_char(array->array[i]));
	}
}

/***********************************************************************************
	SAFE RANDOM
*************************************************************************************/

/* Returns a random number in the range of [0,element-1] */
bi_ptr compute_random_number( bi_ptr result, const bi_ptr element) {
	bi_urandom( result, bi_length( element) + SAFETY_PARAM);
	bi_mod( result, result, element);
	return result;
}

/***********************************************************************************
	SAVE / LOAD
*************************************************************************************/

/* load an big integer from an open file handler */
void bi_load( bi_ptr bi, FILE *file) {
	int i=0;
	char c;

	fgets( buffer, BUFFER_SIZE, file);
	do {
		c = buffer[i];
		i++;
	} while( c != 0 && c != ' ');
	buffer[i-1] = 0;
	bi_set_as_hex( bi, buffer);
}

/* load an big integer array from an open file handler */
void bi_load_array( bi_array_ptr array, FILE *file) {
	int i, j = 0, length;
	char c;

	fgets( buffer, BUFFER_SIZE, file);
	do {
		c = buffer[ j];
		j++;
	} while( c != 0 && c != ' ');
	buffer[ j -1] = 0;
	sscanf( buffer, "%d", &length);
	bi_new_array( array, length);
	for( i=0; i<array->length; i++) {
		bi_load( array->array[i], file);
	}
}

/* save an big integer to an open file handler */
void bi_save( const bi_ptr bi, const char *name, FILE *file) {
	fprintf( file, "%s # %s [%ld]\n", bi_2_hex_char( bi), name, bi_nbin_size( bi));
}

/* save an big integer array to an open file handler */
void bi_save_array( const bi_array_ptr array, const char *name, FILE *file) {
	int i;
	char new_name[100];

	fprintf(file, "%d # %s.length\n", array->length, name);
	for( i=0; i<array->length; i++) {
		sprintf( new_name, "%s[%d]", name, i);
		bi_save( array->array[i], new_name, file);
	}
}

/* convert <bi> to a byte array of length result, the beginning of */
/* this buffer is feel with '0' if needed */
void bi_2_byte_array( unsigned char *result, int length, bi_ptr bi) {
	int i, result_length;

	bi_2_nbin1( &result_length, buffer, bi);
	int delta = length - result_length;
	#ifdef BI_DEBUG
	fprintf( stderr, "[bi_2_byte_array] result_length=%d length=%d\n", result_length, length);
	#endif
	if( delta < 0) {
		LogError( "[bi_2_byte_array] asked length:%d  found:%d\n", length, result_length);
		return;
	}
	for( i=0; i<delta; i++) result[i] = 0;
	for( ; i<length; i++) 	result[ i] = buffer[ i - delta];
}
