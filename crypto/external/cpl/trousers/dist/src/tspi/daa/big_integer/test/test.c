#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <bi.h>

/* for the logging system used by TSS */
setenv("TCSD_FOREGROUND", "1", 1);

 /*
   * standard bit length extension to obtain a uniformly distributed number
   * [0,element]
   */
int test_exp_multi(void);

void foo (bi_t result, const bi_ptr param, unsigned long n) {
	unsigned long i;

	bi_set( result, param);
	bi_mul_si( result, result, n);
	for (i = 1; i < n; i++) bi_add_si( result, result, i*7);
}

void *my_malloc(size_t size) {
	void *ret = malloc( size);

	printf("my_malloc() -> %ld\n", (long)ret);
	return ret;
}

 /**
   * Returns a random number in the range of [0,element-1].
   *
   * @param element
   *          the upper limit
   * @param random
   *          the secure random source
   * @return the random number
   */
void computeRandomNumber(bi_t res, const bi_ptr element) {
	int length = 80 + bi_length( res); // give the length
  	bi_urandom( res, length);  // res = random( of length);
  	int element_length = bi_length( element);
  	bi_mod_si( res, res, element_length); // res = res mod <number byte of element>
}

int main (int argc, char **argv) {
	bi_t bi_tmp, bi_tmp1, bi_tmp2, bi_tmp3, bi_tmp4, bi_tmp5, bi_tmp6;
	bi_t r;
	bi_t n;
	int i;
	unsigned char result1[5];
	int len;
	char *byte_array;
	FILE *file;
	unsigned char ret[] = { (unsigned char)1, 0, (unsigned char)254 };
	int length;
	unsigned char *buffer;
	unsigned char byte;
	bi_ptr nn;

	bi_init( &my_malloc);
	printf("test(%s,%s)\n", __DATE__, __TIME__);
	#ifdef BI_GMP
	printf("using BMP\n");
	#endif
	#ifdef BI_OPENSSL
	printf("using OPENSSL\n");
	#endif

	bi_new( bi_tmp);
	bi_new( bi_tmp1);
	bi_new( bi_tmp2);
	bi_new( bi_tmp3);
	bi_new( bi_tmp4);
	bi_new( bi_tmp5);
	bi_new( bi_tmp6);
	bi_new( n);
	bi_new( r);
	bi_set_as_hex( n, "75E8F38669C531EB78C7ACD62CCDEFFB5E5BE15E2AA55B3AD28B1A35F6E937097CE09A49C689AC335FBA669205CEF209275CFF273F8F81C5B864E5029EECDFA0743BC15D6E4D2C2CB0DED2DC7119A7E0D61669D417BB3B12BA1D10FD40326A49CA6C9E77F8585F25D8C897D9C73284152E103582C018C964F02ADDBA56CB1161A949AAE2847ADE8BC1152716C8B4AF37A87011C2569F646FD3EDA83099048B9525A6401C47A372F3EA43C91066AD5851AE11DEF1EAC7108FFB06AD94D0B849C339A5E8793C4C054456D3D22D30ACCCF7EF33EF7A7D65799E7908D95B0538A9EFC91BF104CE5008D79625394DB1E5883B2F202B95320BBD868BF65C996FC0DFC5");
	bi_set_as_hex( r, "35A624E6607CFD37162C6052547450B2267ECC749F10CDAEB5C294491321EEB47CA0229F423ADCEF3FA7806F5C4DB3C3445D8E7039EBC457149A1343BECF3B1078385C06EE74351A476BE0D5203633C81F7B8D68548DB763F0C096B20615B6016C180291EF32CC064A173BB22F6B46B3240ACC0B50D8338757FA28D5B0313BC4201CD2B35472842E71994C8FCA557B08004B2495304D13A93D796134BB8078E2EE371707DE5809D72474A7CCE1F865ECD8876105D3DB9AFA9426052D0120C755C60F56A0C0F30FAED2053CEB3129FAB6F57F6E209A8E7B2A559D734B339E19E1F2A147BC94DB2FF491CB5ACCEEEED7F2EA75AFF7CAD33E1E420A09135D9C5C1F");
	DUMP_BI( n);
	DUMP_BI( r);
	printf("big number n=n*r\n");
	bi_mul( n, n, r);
	DUMP_BI( n);
	bi_set_as_hex( r, "D7E7028DA181DADAC29C95143C865702453465115AFA7576AADF1E57DD84DA7FF4C8F66530D1E9D1AB69BC12342B89FA0A9755F9F4EE1DA445D50016CEF50622ED905CC9B987FCC7910CAA841641814C1994BC442A15CB05FE5C145626F1454E90435FBC6A529856EF29BDBCBFCB62FB69EDBD11DC33357667867278E1679EABCDBEEA02E9A6911804DF47ACA6B2D63A31E258AD542D71A8178A5E072F5E221EADBB10E16D5533AE427101FF94C5967575FABCD18305C5F15C103CEA1A8ACD01898E88426EDA7C0DF58AA48435808A840F6EEE1D7205D33F356E20FE0D4136B401BF386F11869C3CE4A808B96435694748EF3706F58756548A71E4CF4D2BE157");
	bi_mod( n, n, r);
	printf("mod big number n=n mod r\n");
	DUMP_BI( n);
	if( bi_get_si( bi_set_as_si( n, 13)) != 13) {
		printf("!!! bi_set_as_si 13(%s) = 13\n", bi_2_dec_char( n ));
		exit(-1);
	}
	if( bi_get_si( bi_set_as_si( n, -13)) != -13) {
		printf("!!! bi_set_as_si -13(%s) = -13\n", bi_2_dec_char( n ));
		exit(-1);
	}
	if( bi_get_si( bi_inc(bi_set_as_si( n, 13))) != 14) {
		puts("!!! bi_inc 13++ = 14\n");
		exit(-1);
	}
	if( bi_get_si( bi_dec(bi_set_as_si( n, 13))) != 12) {
		puts("!!! bi_dec 13-- = 12\n");
		exit(-1);
	}
	if( bi_get_si( bi_setbit(bi_set_as_si( n, 0), 10)) != 1024) {
		puts("!!! bi_setbit set[10] = 1024\n");
		exit(-1);
	}
	if( bi_get_si( bi_mod_si(bi_tmp, bi_set_as_si( n, 12), 10)) != 2) {
		puts("!!! bi_mod_si 12 mod 10 = 2\n");
		exit(-1);
	}
	if( bi_get_si( bi_mul_si(bi_tmp, bi_set_as_si( n, 12), 10)) != 120) {
		puts("!!! bi_mul_si 12 * 10 = 120\n");
		exit(-1);
	}
	if( bi_get_si( bi_mul(bi_tmp, bi_set_as_si( n, 12), bi_set_as_si( bi_tmp1, 10))) != 120) {
		puts("!!! bi_mul_si 12 * 10 = 120\n");
		exit(-1);
	}
	if( bi_get_si( bi_mod_exp_si(bi_tmp, bi_set_as_si( bi_tmp1, 4), bi_2, 10)) != 6) {
		puts("!!! bi_mod_exp_si 4 ^ 2 mod 10 = 6\n");
		exit(-1);
	}
	if( bi_get_si( bi_mod_exp(bi_tmp, bi_set_as_si( bi_tmp1, 4), bi_2, bi_set_as_si( bi_tmp2, 10))) != 6) {
		puts("!!! bi_mod_exp 4 ^ 2 mod 10 = 6\n");
		exit(-1);
	}
	if( bi_get_si( bi_mod(bi_tmp, bi_set_as_si( n, 12), bi_set_as_si(bi_tmp1, 10))) != 2) { printf("!!! bi_mod 12 mod 10 = 2 [%s]\n",bi_2_dec_char(  bi_tmp)); exit(-1); }
	if( bi_get_si( bi_mod(bi_tmp, bi_set_as_si( n, -12), bi_set_as_si(bi_tmp1, 10))) != 8) { printf("!!! bi_mod -12 mod 10 = 8 [%s]\n",bi_2_dec_char(  bi_tmp)); exit(-1); }
	if( bi_get_si( bi_mod(bi_tmp, bi_set_as_si( n, -27), bi_set_as_si(bi_tmp1, 10))) != 3) { printf("!!! bi_mod -27 mod 10 = 3 [%s]\n",bi_2_dec_char(  bi_tmp)); exit(-1); }
	bi_set_as_si(n, 0x12345678);
	bi_2_byte_array( result1, 5, n);
	if( result1[0] != 0x00  || result1[1] != 0x12 ||  result1[2] != 0x34 || result1[3] != 0x56 ||  result1[4] != 0x78 ) {
		printf("!!! bi_2_byte_array[0x123456578] [0]=%x [1]=%x [2]=%x [3]=%x [4]=%x \n", result1[0], result1[1], result1[2], result1[3], result1[4]);
		exit( -1);
	}
	byte_array = retrieve_byte_array( &len, "12345");
	printf("test dump_byte_array len=%d \n", len);
	printf("test dump_byte_array(\"12345\")=%s\n", dump_byte_array( len, byte_array));
	free( byte_array);
	byte_array = retrieve_byte_array( &len, "12345678");
	printf("test dump_byte_array len=%d \n", len);
	printf("test dump_byte_array(\"12345678\")=%s\n", dump_byte_array( len, byte_array));

	// test save end load of bi_t and bi_array
	/////////////////////////////////////////////////////////////////////////////////////
	bi_array result;
	bi_new_array( result, 2);
	bi_set_as_si( bi_tmp, 6);
	bi_set_as_si( result->array[0], 314159);
	bi_set_as_si( result->array[1], 123456789);
	file = fopen("/tmp/test.todel", "w");
	bi_save_array( result, "result", file);
	bi_save( bi_tmp, "bi_tmp", file);
	fclose( file);
	bi_set_as_si( result->array[0], 0);
	bi_set_as_si( result->array[1], 0);
	bi_set_as_si( bi_tmp, 0);
	file = fopen("/tmp/test.todel", "r");
	bi_load_array( result, file);
	bi_load( bi_tmp, file);
	fclose( file);
	if( bi_get_si( result->array[0]) != 314159) { puts("!!! save/load array[0] = 314159\n"); exit(-1); }
	if( bi_get_si( result->array[1]) != 123456789) { puts("!!! save/load array[1] = 123456789\n"); exit(-1); }
	if( bi_get_si( bi_tmp) != 6) { puts("!!! save/load bi_tmp = 6\n"); exit(-1); }

	// conversion from bi_t 2 big endian BYTE*
	/////////////////////////////////////////////////////////////////////////////////////
	bi_set_as_si( n, 254+(1 << 16));
	buffer = bi_2_nbin( &length, n);
	printf("value 2 convert=%s  length=%ld\n", bi_2_hex_char( n), bi_nbin_size( n));
	for( i=0; i<length; i++) {
		byte = (unsigned char)(buffer[i] & 0xFF);
		if( byte != ret[i]) { printf("\n!!! bi_2_nbin[%d] %x = %x\n", i, byte, ret[i]); exit(-1); }
		printf("[buffer[%d]=%x]", i, (int)(byte));
	}
	printf("\n");
	nn = bi_set_as_nbin( length, buffer);
	if( bi_equals_si( nn, 254+(1 << 16) ) == 0) {
		printf("\n!!! bi_set_as_nbin %s = %x\n", bi_2_hex_char( nn), 254+(1 << 16));
		exit(-1);
	}
	if( !bi_equals( bi_sub_si( bi_tmp, bi_set_as_si( bi_tmp1, 9), 1),
				bi_mod( bi_tmp2,
					bi_mul( bi_tmp3, bi_set_as_si(bi_tmp4, 6),
						bi_set_as_si( bi_tmp5, 3)),
					bi_set_as_si( bi_tmp6, 10)))) {
		puts("!!! 9-1 == (6*3) mod 10\n");
		printf("!!! tmp(8) = %s tmp1(9)=%s tmp2(8)=%s tmp3(6*3)=%s tmp4(6)=%s\
 tmp5(3)=%s tmp6(10)=%s\n",
			bi_2_dec_char(bi_tmp),
			bi_2_dec_char(bi_tmp1),
			bi_2_dec_char(bi_tmp2),
			bi_2_dec_char(bi_tmp3),
			bi_2_dec_char(bi_tmp4),
			bi_2_dec_char(bi_tmp5),
			bi_2_dec_char(bi_tmp6));
		exit(-1);
		puts("!!! 9-1 == (6*3) mod 10\n"); exit(-1);
	}
	bi_set_as_si(n, 1);
	bi_shift_left(n, n, 10);
	printf("1 << 10 = %s\n", bi_2_dec_char( n));
	bi_set_as_si(n, 1);
	printf("(1 << 10) >> 5 = %s\n", bi_2_dec_char( bi_shift_right(bi_tmp, bi_shift_left(bi_tmp1, n, 10), 5)));
	bi_set_as_si(n, 1);
	printf("[* (1 << 10) >> 5 *] = (2^10) / (2^5)  -> %s\n", bi_2_dec_char( bi_shift_right( bi_tmp, ( bi_shift_left( bi_tmp1, n, 10)), 5)));
	bi_set_as_si( n, 10);
	printf("   (2^10) = %s\n", bi_2_dec_char( bi_mod_exp_si( bi_tmp, bi_2, n, 2000)));
	printf("   (1<<5) = %s\n", bi_2_dec_char( bi_shift_left( bi_tmp, bi_set_as_si( bi_tmp1, 1), 5)));
	printf("   1024 / 500 = %s\n", bi_2_dec_char( bi_div( bi_tmp, bi_set_as_si( bi_tmp1, 1024), bi_set_as_si( bi_tmp2, 500))));
	printf("   1024 / 500 = %s\n", bi_2_dec_char( bi_div_si( bi_tmp, bi_set_as_si( bi_tmp1, 1024), 500)));
	printf("   (1 << 10) >> 5 = [* (2^10) / (2^5) *] -> %s\n", bi_2_dec_char( bi_div( bi_tmp1,
															bi_mod_exp_si( bi_tmp2, bi_2, bi_set_as_si( bi_tmp3, 10), 2000),
															bi_mod_exp_si( bi_tmp4, bi_2, bi_set_as_si( bi_tmp5,   5), 2000) )));

	printf("(1 << 10) >> 5 = (2^10) / (1<<5)  -> %d\n", bi_equals( bi_shift_right( bi_tmp, ( bi_shift_left( bi_tmp1, bi_1, 10)), 5),
													bi_div( bi_tmp2,
														bi_mod_exp_si( bi_tmp3, bi_2, bi_set_as_si( bi_tmp4, 10), 2000),
														bi_mod_exp_si( bi_tmp5, bi_2, bi_set_as_si( bi_tmp6,   5), 2000))));
	printf("( 45 ^ -5 ) %% 10 == ( 1 / ( (45 ^ 5) %% 10) ) %% 10\n");
	bi_set_as_si( bi_tmp, 45);
	bi_set_as_si( bi_tmp1, 5);
	// bi_negate( bi_tmp1);
	bi_set_as_si( bi_tmp2, 10);
	bi_mod_exp( bi_tmp3, bi_tmp, bi_tmp1, bi_tmp2);
	bi_set_as_si( bi_tmp1, 5);
	bi_mod_exp( bi_tmp4, bi_tmp, bi_tmp1, bi_tmp2);
	printf("\t( 45 ^ -5 ) %% 10 = %s\n", bi_2_dec_char( bi_tmp3));
	printf("\t( 1 / ( (45 ^ 5) %% 10) ) %% 10 = %s\n", bi_2_dec_char( bi_tmp4));
	if( bi_equals( bi_tmp3, bi_tmp4) == 0) {
		printf("!!! error !\n");
		exit( -1);
	}
	for( i=0; i<5; i++) {
		bi_generate_prime( bi_tmp, 1024);
		printf("bi=%s\n", bi_2_hex_char( bi_tmp));
		printf("bi.length=%ld \n", bi_length( bi_tmp));
		if( bi_length( bi_tmp) != 1024) { puts("!!! length(random(1024)) != 1024\n"); exit(-1); }
	}
	bi_set_as_si(n, 0);
	bi_setbit( n, 10);
	printf("setbit(10) = %s\n", bi_2_dec_char( n));
	bi_set_as_dec(n, "123456");
	foo( r, n, 20L);
	printf("TEST:%s\n", bi_2_dec_char( r));
	bi_urandom( n, 1024);
	bi_urandom( r, 1024);
	computeRandomNumber( r, n);
	printf("r:%s  n:%s\n", bi_2_hex_char( r), bi_2_hex_char( n));
	bi_generate_prime( r, 1024);
	printf("prime:%s\nIs probable prime:%d\n", bi_2_hex_char( r), bi_is_probable_prime( r));
	int error = bi_invert_mod( r, r, n);
	printf("Invert mod return:%d\nInvert(r):%s\n", error, bi_2_hex_char( r));
	bi_negate( r);
	printf("negate(r):%s\n", bi_2_hex_char( r));
	bi_generate_safe_prime( r, 128);
	bi_sub_si( n, r, 1);       // n= r - 1
	bi_shift_right( n, n, 1); // n = n / 2
	printf("safe prime(r):%s probable_prime:%d\n", bi_2_hex_char( r), bi_is_probable_prime( r));
	printf("safe prime( (r-1)/2):%s probable_prime:%d\n", bi_2_hex_char( n), bi_is_probable_prime( n));
	test_exp_multi();
	bi_free( r);
	bi_free( n);

	bi_set_as_si( result->array[0], 1);
	printf("result-> 1<<20:%s\n", bi_2_dec_char( bi_shift_left( result->array[0], result->array[0], 20)));
	bi_set_as_si( result->array[1], 1);
	printf("result1-> 1<<10:%s\n", bi_2_dec_char( bi_shift_right( result->array[1], result->array[0], 10)));
	// copy arrays
	bi_array new_result; bi_new_array2( new_result, 4);
	bi_new_array2( new_result, 4);
	bi_copy_array( result, 0, new_result, 0, 2);
	bi_copy_array( result, 0, new_result, 2, 2);
 	for( i = 0; i<4; i++) {
		printf("new_result[%d]-> [even-> 1<<20] [odd-> 1<<10] :%s\n", i, bi_2_dec_char( new_result->array[i]));
 	}

	bi_free( bi_tmp);
	bi_free( bi_tmp1);
	bi_free( bi_tmp2);
	bi_free( bi_tmp3);
	bi_free( bi_tmp4);
	bi_free( bi_tmp5);
	bi_free( bi_tmp6);
	bi_release();
	printf("THE END [%s,%s]\n", __DATE__, __TIME__);
	fflush(stdout);
	return 0;

}

