/*	$NetBSD: writekey.c,v 1.1.1.1.2.2 2009/05/13 18:50:30 jym Exp $	*/

/* writekey [-s $slot] [-p $pin] -l $label -i $id -f $filename */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#ifndef OPENCRYPTOKI
#include <security/cryptoki.h>
#include <security/pkcs11.h>
#else
#include <opencryptoki/pkcs11.h>
#endif
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

/* Define static key template values */
static CK_BBOOL truevalue = TRUE;
static CK_BBOOL falsevalue = FALSE;

int
main(int argc, char *argv[])
{
    ENGINE *e;
    RSA *rsa = NULL;
    FILE *fp;
    CK_RV rv;
    CK_SLOT_ID slot = 0;
    CK_SESSION_HANDLE hSession;
    CK_UTF8CHAR *pin = NULL;
    CK_BYTE new_id[2];
    CK_OBJECT_HANDLE key = CK_INVALID_HANDLE;
    CK_OBJECT_CLASS kclass;
    CK_KEY_TYPE ktype = CKK_RSA;
    CK_ATTRIBUTE template[50];
    CK_ULONG template_size;
    CK_BYTE data[8][1024];
    CK_ULONG ulObjectCount;
    char *label = NULL, *filename = NULL;
    int id = 0;
    int error = 0;
    int c, errflg = 0;
    extern char *optarg;
    extern int optopt;
    
    while ((c = getopt(argc, argv, ":s:l:i:p:f:")) != -1) {
        switch (c) {
	case 's':
	    slot = atoi(optarg);
	    break;
        case 'l':
            label = optarg;
            break;
        case 'i':
            id = atoi(optarg);
	    id &= 0xffff;
            break;
        case 'p':
            pin = (CK_UTF8CHAR *)optarg;
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
    if ((errflg) || (!label) || (!id) || (!filename)) {
        fprintf(stderr,
		"usage: writekey [-s slot] [-p pin] -l label -i id "
		"-f filename\n");
        exit(2);
    }
    
    /* Load the config file */
    OPENSSL_config(NULL);

    /* Register engine */
    e = ENGINE_by_id("pkcs11");
    if (!e) {
        /* the engine isn't available */
        printf("The engine isn't available\n");
	ERR_print_errors_fp(stderr);
        exit(1);
    }
    
    if (!ENGINE_init(e)) {
        /* the engine couldn't initialise, release 'e' */
        printf("The engine couldn't initialise\n");
	ERR_print_errors_fp(stderr);
        ENGINE_free(e);
        exit(1);
    }

    /* Read the key */
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening input file.\n");
	ENGINE_free(e);
	exit(1);
    }

    rsa = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
    (void) fclose(fp);
    if (rsa == NULL) {
        printf("Error reading input file.\n");
	ERR_print_errors_fp(stderr);
	ENGINE_free(e);
	exit(1);
    }

    /* Initialize the CRYPTOKI library */
    rv = C_Initialize(NULL_PTR);
    if ((rv != CKR_OK) && (rv != CKR_CRYPTOKI_ALREADY_INITIALIZED)) {
        fprintf(stderr, "C_Initialize: Error = 0x%.8X\n", rv);
	ENGINE_free(e);
	exit(1);
    }

    /* Open a session on the slot found */
    rv = C_OpenSession(slot, CKF_RW_SESSION+CKF_SERIAL_SESSION,
		       NULL_PTR, NULL_PTR, &hSession);
    if (rv != CKR_OK) {
	fprintf(stderr, "C_OpenSession: Error = 0x%.8X\n", rv);
	error = 1;
	goto exit_program;
    }

    /* Login to the Token (Keystore) */
    if (!pin)
#ifndef OPENCRYPTOKI
        pin = (CK_UTF8CHAR *)getpassphrase("Enter Pin: ");
#else
        pin = (CK_UTF8CHAR *)getpass("Enter Pin: ");
#endif
    rv = C_Login(hSession, CKU_USER, pin, strlen((char *)pin));
    memset(pin, 0, strlen((char *)pin));
    if (rv != CKR_OK) {
	fprintf(stderr, "C_Login: Error = 0x%.8X\n", rv);
        error = 1;
        goto exit_session;
    }
   
    /* fill the search template */
    if (strstr(label, "pkcs11:") == label)
        label = strstr(label, ":") + 1;
    kclass = CKO_PRIVATE_KEY;
    template[0].type = CKA_TOKEN;
    template[0].pValue = &truevalue;
    template[0].ulValueLen = sizeof (truevalue);
    template[1].type = CKA_CLASS;
    template[1].pValue = &kclass;
    template[1].ulValueLen = sizeof (kclass);
    template[2].type = CKA_LABEL;
    template[2].pValue = label;
    template[2].ulValueLen = strlen(label);

    /* check if a key with the same label already exists */
    rv = C_FindObjectsInit(hSession, template, 3); 
    if (rv != CKR_OK) {
        fprintf(stderr, "C_FindObjectsInit: Error = 0x%.8X\n", rv);
        error = 1;
        goto exit_session;
    }
    rv = C_FindObjects(hSession, &key, 1, &ulObjectCount);
    if (rv != CKR_OK) {
        fprintf(stderr, "C_FindObjects: Error = 0x%.8X\n", rv);
        error = 1;
        goto exit_search;
    }
    if (ulObjectCount != 0) {
        fprintf(stderr, "Key already exists.\n");
        error = 1;
        goto exit_search;
    }
    
    /* fill attributes for the public key */
    new_id[0] = (id >> 8) & 0xff;
    new_id[1] = id & 0xff;
    kclass = CKO_PUBLIC_KEY;
    if (BN_num_bytes(rsa->n) > 1024) {
        fprintf(stderr, "RSA modulus too large\n");
	error = 1;
	goto exit_search;
    }
    BN_bn2bin(rsa->n, data[0]);
    if (BN_num_bytes(rsa->e) > 1024) {
        fprintf(stderr, "RSA public exponent too large\n");
	error = 1;
	goto exit_search;
    }
    BN_bn2bin(rsa->e, data[1]);
    if (BN_num_bytes(rsa->d) > 1024) {
        fprintf(stderr, "RSA private exponent too large\n");
	error = 1;
	goto exit_search;
    }
    BN_bn2bin(rsa->d, data[2]);
    if (BN_num_bytes(rsa->p) > 1024) {
        fprintf(stderr, "RSA prime 1 too large\n");
	error = 1;
	goto exit_search;
    }
    BN_bn2bin(rsa->p, data[3]);
    if (BN_num_bytes(rsa->q) > 1024) {
        fprintf(stderr, "RSA prime 2 too large\n");
	error = 1;
	goto exit_search;
    }
    BN_bn2bin(rsa->q, data[4]);
    if (BN_num_bytes(rsa->dmp1) > 1024) {
        fprintf(stderr, "RSA exponent 1 too large\n");
	error = 1;
	goto exit_search;
    }
    BN_bn2bin(rsa->dmp1, data[5]);
    if (BN_num_bytes(rsa->dmq1) > 1024) {
        fprintf(stderr, "RSA exponent 2 too large\n");
	error = 1;
	goto exit_search;
    }
    BN_bn2bin(rsa->dmq1, data[6]);
    if (BN_num_bytes(rsa->iqmp) > 1024) {
        fprintf(stderr, "RSA coefficient too large\n");
	error = 1;
	goto exit_search;
    }
    BN_bn2bin(rsa->iqmp, data[7]);

    template[0].type = CKA_TOKEN;
    template[0].pValue = &truevalue;
    template[0].ulValueLen = sizeof (truevalue);
    template[1].type = CKA_CLASS;
    template[1].pValue = &kclass;
    template[1].ulValueLen = sizeof (kclass);
    template[2].type = CKA_LABEL;
    template[2].pValue = label;
    template[2].ulValueLen = strlen(label);
    template[3].type = CKA_ID;
    template[3].pValue = new_id;
    template[3].ulValueLen = sizeof (new_id);
    template[4].type = CKA_KEY_TYPE;
    template[4].pValue = &ktype;
    template[4].ulValueLen = sizeof (ktype);
    template[5].type = CKA_ENCRYPT;
    template[5].pValue = &truevalue;
    template[5].ulValueLen = sizeof (truevalue);
    template[6].type = CKA_VERIFY;
    template[6].pValue = &truevalue;
    template[6].ulValueLen = sizeof (truevalue);
    template[7].type = CKA_VERIFY_RECOVER;
    template[7].pValue = &truevalue;
    template[7].ulValueLen = sizeof (truevalue);
    template[8].type = CKA_MODULUS;
    template[8].pValue = data[0];
    template[8].ulValueLen = BN_num_bytes(rsa->n);
    template[9].type = CKA_PUBLIC_EXPONENT;
    template[9].pValue = data[1];
    template[9].ulValueLen = BN_num_bytes(rsa->e);

    rv = C_CreateObject(hSession, template, 10, &key);
    if (rv != CKR_OK) {
        fprintf(stderr, "C_CreateObject (pub): Error = 0x%.8X\n", rv);
	error = 1;
	goto exit_search;
    }

    /* fill attributes for the private key */
    kclass = CKO_PRIVATE_KEY;
    template[0].type = CKA_TOKEN;
    template[0].pValue = &truevalue;
    template[0].ulValueLen = sizeof (truevalue);
    template[1].type = CKA_CLASS;
    template[1].pValue = &kclass;
    template[1].ulValueLen = sizeof (kclass);
    template[2].type = CKA_LABEL;
    template[2].pValue = label;
    template[2].ulValueLen = strlen(label);
    template[3].type = CKA_ID;
    template[3].pValue = new_id;
    template[3].ulValueLen = sizeof (new_id);
    template[4].type = CKA_KEY_TYPE;
    template[4].pValue = &ktype;
    template[4].ulValueLen = sizeof (ktype);
    template[5].type = CKA_SENSITIVE;
    template[5].pValue = &falsevalue;
    template[5].ulValueLen = sizeof (falsevalue);
    template[6].type = CKA_EXTRACTABLE;
    template[6].pValue = &truevalue;
    template[6].ulValueLen = sizeof (truevalue);
    template[7].type = CKA_DECRYPT;
    template[7].pValue = &truevalue;
    template[7].ulValueLen = sizeof (truevalue);
    template[8].type = CKA_SIGN;
    template[8].pValue = &truevalue;
    template[8].ulValueLen = sizeof (truevalue);
    template[9].type = CKA_SIGN_RECOVER;
    template[9].pValue = &truevalue;
    template[9].ulValueLen = sizeof (truevalue);
    template[10].type = CKA_MODULUS;
    template[10].pValue = data[0];
    template[10].ulValueLen = BN_num_bytes(rsa->n);
    template[11].type = CKA_PUBLIC_EXPONENT;
    template[11].pValue = data[1];
    template[11].ulValueLen = BN_num_bytes(rsa->e);
    template[12].type = CKA_PRIVATE_EXPONENT;
    template[12].pValue = data[2];
    template[12].ulValueLen = BN_num_bytes(rsa->d);
    template[13].type = CKA_PRIME_1;
    template[13].pValue = data[3];
    template[13].ulValueLen = BN_num_bytes(rsa->p);
    template[14].type = CKA_PRIME_2;
    template[14].pValue = data[4];
    template[14].ulValueLen = BN_num_bytes(rsa->q);
    template[15].type = CKA_EXPONENT_1;
    template[15].pValue = data[5];
    template[15].ulValueLen = BN_num_bytes(rsa->dmp1);
    template[16].type = CKA_EXPONENT_2;
    template[16].pValue = data[6];
    template[16].ulValueLen = BN_num_bytes(rsa->dmq1);
    template[17].type = CKA_COEFFICIENT;
    template[17].pValue = data[7];
    template[17].ulValueLen = BN_num_bytes(rsa->iqmp);

    rv = C_CreateObject(hSession, template, 18, &key);
    if (rv != CKR_OK) {
        fprintf(stderr, "C_CreateObject (priv): Error = 0x%.8X\n", rv);
	(void) C_DestroyObject(hSession, key);
	error = 1;
	goto exit_search;
    }

 exit_search:
    rv = C_FindObjectsFinal(hSession);
    if (rv != CKR_OK) {
        fprintf(stderr, "C_FindObjectsFinal: Error = 0x%.8X\n", rv);
        error = 1;
    }

 exit_session:
    (void) C_CloseSession(hSession);

 exit_program:
    (void) C_Finalize(NULL_PTR);
    ENGINE_free(e);
    ENGINE_cleanup();

    exit(error);
}
