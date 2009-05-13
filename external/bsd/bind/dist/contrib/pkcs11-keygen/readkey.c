/*	$NetBSD: readkey.c,v 1.1.1.1.2.2 2009/05/13 18:50:30 jym Exp $	*/

/* readkey [-s $slot] -l $label [-p $pin] -f $filename */

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
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

static CK_BBOOL truevalue = TRUE;

int
main(int argc, char *argv[])
{
    RSA *rsa = NULL;
    FILE *fp;
    CK_RV rv;
    CK_SLOT_ID slot = 0;
    CK_SESSION_HANDLE hSession;
    CK_UTF8CHAR *pin = NULL;
    char *label;
    CK_OBJECT_HANDLE key = CK_INVALID_HANDLE;
    CK_OBJECT_CLASS kclass = CKO_PRIVATE_KEY;
    char *filename;
    int error = 0;
    int i = 0;
    int c, errflg = 0;
    CK_ULONG ulObjectCount;
    CK_ATTRIBUTE search_template[] = {
	{CKA_LABEL, NULL, 0},
        {CKA_TOKEN, &truevalue, sizeof (truevalue)},
	{CKA_CLASS, &kclass, sizeof (kclass)}
    };
    CK_BYTE id[32];
    CK_BYTE data[8][1024];
    CK_ATTRIBUTE attr_template[] = {
        {CKA_ID, &id, sizeof (id)},
	{CKA_MODULUS, (void *)data[0], 1024},          /* n */
	{CKA_PUBLIC_EXPONENT, (void *)data[1], 1024},  /* e */
	{CKA_PRIVATE_EXPONENT, (void *)data[2], 1024}, /* d */
	{CKA_PRIME_1, (void *)data[3], 1024},          /* p */
	{CKA_PRIME_2, (void *)data[4], 1024},          /* q */
	{CKA_EXPONENT_1, (void *)data[5], 1024},       /* dmp1 */
	{CKA_EXPONENT_2, (void *)data[6], 1024},       /* dmq1 */
	{CKA_COEFFICIENT, (void *)data[7], 1024}       /* iqmp */
    };
    extern char *optarg;
    extern int optopt;

    while ((c = getopt(argc, argv, ":s:l:p:f:")) != -1) {
        switch (c) {
	case 's':
	    slot = atoi(optarg);
	    break;
        case 'l':
            label = optarg;
            break;
        case 'p':
            pin = (CK_UTF8CHAR *)optarg;
            break;
        case ':':
            fprintf(stderr, "Option -%c requires an operand\n", optopt);
            errflg++;
            break;
	case 'f':
	    filename = optarg;
	    break;
        case '?':
	default:
            fprintf(stderr, "Unrecognised option: -%c\n", optopt);
            errflg++;
        }
    }
    if ((errflg) || (!label) || (!filename)) {
        fprintf(stderr,
		"usage: readkey [-s slot] -l label [-p pin] -f filename\n");
        exit(1);
    }
    if (slot)
        printf("slot %d\n", slot);

    /* Initialize OpenSSL library */
    OPENSSL_config(NULL);
    rsa = RSA_new();
    if (!rsa) {
        fprintf(stderr, "RSA_new failed\n");
	ERR_print_errors_fp(stderr);
	exit(1);
    }

    /* Initialize the CRYPTOKI library */
    rv = C_Initialize(NULL_PTR);
    if ((rv != CKR_OK) && (rv != CKR_CRYPTOKI_ALREADY_INITIALIZED)) {
        fprintf(stderr, "C_Initialize: Error = 0x%.8X\n", rv);
        exit(1);
    }

    /* Open a session on the slot found */
    rv = C_OpenSession(slot, CKF_SERIAL_SESSION,
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

    /* Set search template. */
    if (strstr(label, "pkcs11:") == label)
        label = strstr(label, ":") + 1;
    search_template[0].pValue = label;
    search_template[0].ulValueLen = strlen(label);

    rv = C_FindObjectsInit(hSession, search_template, 3); 
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
    if (ulObjectCount == 0) {
        fprintf(stderr, "C_FindObjects: can't find the key\n");
	error = 1;
	goto exit_search;
    }

    rv = C_GetAttributeValue(hSession, key, attr_template, 9);
    if (rv != CKR_OK) {
        fprintf(stderr, "C_GetAttributeValue: Error = 0x%.8X\n", rv);
	error = 1;
	goto exit_search;
    }

    printf("ID[%u]=", attr_template[0].ulValueLen);
    for (i = 0; i < attr_template[0].ulValueLen; i++)
        printf("%02x", id[i]);
    printf("\n");

    if (attr_template[1].ulValueLen > 0)
        rsa->n = BN_bin2bn(data[0], attr_template[1].ulValueLen, NULL);
    if (attr_template[2].ulValueLen > 0)
        rsa->e = BN_bin2bn(data[1], attr_template[2].ulValueLen, NULL);
    if (attr_template[3].ulValueLen > 0)
        rsa->d = BN_bin2bn(data[2], attr_template[3].ulValueLen, NULL);
    if (attr_template[4].ulValueLen > 0)
        rsa->p = BN_bin2bn(data[3], attr_template[4].ulValueLen, NULL);
    if (attr_template[5].ulValueLen > 0)
        rsa->q = BN_bin2bn(data[4], attr_template[5].ulValueLen, NULL);
    if (attr_template[6].ulValueLen > 0)
        rsa->dmp1 = BN_bin2bn(data[5], attr_template[6].ulValueLen, NULL);
    if (attr_template[7].ulValueLen > 0)
        rsa->dmq1 = BN_bin2bn(data[6], attr_template[7].ulValueLen, NULL);
    if (attr_template[8].ulValueLen > 0)
        rsa->iqmp = BN_bin2bn(data[7], attr_template[8].ulValueLen, NULL);

    rv = C_FindObjects(hSession, &key, 1, &ulObjectCount);
    if (rv != CKR_OK) {
        fprintf(stderr, "C_FindObjects: Error = 0x%.8X\n", rv);
	error = 1;
	goto exit_search;
    }
    if (ulObjectCount != 0) {
        fprintf(stderr, "C_FindObjects: found extra keys?\n");
	error = 1;
	goto exit_search;
    }

    printf("RSA=");
    RSA_print_fp(stdout, rsa, 4);

    fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Error opening output file.\n");
	error = 1;
	goto exit_search;
    }

    if (!PEM_write_RSAPrivateKey(fp, rsa, NULL, NULL, 0, NULL, NULL)) {
        printf("Error writing output file.\n");
	ERR_print_errors_fp(stderr);
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

    exit(error);
}
