/*	$NetBSD: PEM_write_pubkey.c,v 1.1.1.1.2.2 2009/05/13 18:50:30 jym Exp $	*/

/* OpenSSL tool
 *
 * usage: PEM_write_pubkey -e engine -p pin -k keyname -f filename
 */

#include <stdio.h>
#include <stdlib.h>
#include <openssl/engine.h>
#include <openssl/conf.h>
#include <unistd.h>
#include <errno.h>

extern int PEM_write_PUBKEY(FILE *fp, EVP_PKEY *x);

int
main(int argc, char *argv[])
{
    ENGINE *e;
    EVP_PKEY *pub_key;
    FILE *fp;
    char *engine = NULL;
    char *pin = NULL;
    char *keyname = NULL;
    char *filename = NULL;
    int c, errflg = 0;
    extern char *optarg;
    extern int optopt;
       
    while ((c = getopt(argc, argv, ":e:p:k:f:")) != -1) {
        switch (c) {
        case 'e':
            engine = optarg;
            break;
	case 'p':
	    pin = optarg;
	    break;
        case 'k':
            keyname = optarg;
            break;
        case 'f':
            filename = optarg;
            break;
        case ':':
            fprintf(stderr, "Option -%c requires an operand\n", optopt);
            errflg++;
            break;
        case '?':
	default:
            fprintf(stderr, "Unrecognised option: -%c\n", optopt);
            errflg++;
        }
    }
    if ((errflg) || (!engine) || (!filename) || (!keyname)) {
        fprintf(stderr,
		"usage: PEM_write_pubkey -e engine [-p pin] "
		"-k keyname -f filename\n");
        exit(1);
    }
    
    /* Load the config file */
    OPENSSL_config(NULL);

    /* Register engine */
    e = ENGINE_by_id(engine);
    if (!e) {
        /* the engine isn't available */
        printf("The engine isn't available\n");
	ERR_print_errors_fp(stderr);
        exit(1);
    }
    
    /* Send PIN to engine */
    if (pin && !ENGINE_ctrl_cmd_string(e, "PIN", pin, 0)){
        printf("Error sending PIN to engine\n");
	ERR_print_errors_fp(stderr);
        ENGINE_free(e);
        exit(1);
    }
    
    if (!ENGINE_init(e)) {
        /* the engine couldn't initialise, release 'e' */
        printf("The engine couldn't initialise\n");
	ERR_print_errors_fp(stderr);
        ENGINE_free(e);
        exit(1);
    }

    if (!ENGINE_register_RSA(e)){
        /* This should only happen when 'e' can't initialise, but the previous
	 * statement suggests it did. */
        printf("This should not happen\n");
	ERR_print_errors_fp(stderr);
        exit(1);
    }
    
    /* Load public key */
    pub_key = ENGINE_load_public_key(e, keyname, NULL, NULL);
    if (pub_key == NULL) {
        /* No public key */
        printf("Error loading public key\n");
	ERR_print_errors_fp(stderr);
        ENGINE_free(e);
        exit(1);
    }

    /* write public key to file in PEM format */
    fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Error opening output file.\n");
	ENGINE_free(e);
        exit(1);
    }
     
    if (!PEM_write_PUBKEY(fp, pub_key)) {
        /* Error writing public key */
        printf("Error writing public key");
	ERR_print_errors_fp(stderr);
        ENGINE_free(e);
        exit(1);
    }
    
    fclose(fp);
    exit(0);
}
