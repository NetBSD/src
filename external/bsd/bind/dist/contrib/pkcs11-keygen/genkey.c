/*	$NetBSD: genkey.c,v 1.1.1.1.2.2 2009/05/13 18:50:30 jym Exp $	*/

/* genkey - pkcs11 rsa key generator
 *
 * create RSASHA1 key in the keystore of an SCA6000
 * The calculation of key tag is left to the script
 * that converts the key into a DNSKEY RR and inserts 
 * it into a zone file.
 *
 * usage:
 * genkey [-P] [-s slot] -b keysize -l label [-p pin] 
 *
 */

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

/* Define static key template values */
static CK_BBOOL truevalue = TRUE;
static CK_BBOOL falsevalue = FALSE;

int
main(int argc, char *argv[])
{
    CK_RV rv;
    CK_SLOT_ID slot = 0;
    CK_MECHANISM genmech;
    CK_SESSION_HANDLE hSession;
    CK_UTF8CHAR *pin = NULL;
    CK_ULONG modulusbits = 0;
    CK_CHAR *label = NULL;
    CK_OBJECT_HANDLE privatekey, publickey;
    CK_BYTE public_exponent[3];
    int error = 0;
    int i = 0;
    int c, errflg = 0;
    int hide = 1;
    CK_ULONG ulObjectCount;
    /* Set search template */
    CK_ATTRIBUTE search_template[] = {
        {CKA_LABEL, NULL_PTR, 0}
    };
    CK_ATTRIBUTE publickey_template[] = {
        {CKA_LABEL, NULL_PTR, 0},
        {CKA_VERIFY, &truevalue, sizeof (truevalue)},
        {CKA_TOKEN, &truevalue, sizeof (truevalue)},
        {CKA_MODULUS_BITS, &modulusbits, sizeof (modulusbits)},
        {CKA_PUBLIC_EXPONENT, &public_exponent, sizeof (public_exponent)}
    };
    CK_ATTRIBUTE privatekey_template[] = {
        {CKA_LABEL, NULL_PTR, 0},
        {CKA_SIGN, &truevalue, sizeof (truevalue)},
        {CKA_TOKEN, &truevalue, sizeof (truevalue)},
        {CKA_PRIVATE, &truevalue, sizeof (truevalue)},
        {CKA_SENSITIVE, &truevalue, sizeof (truevalue)},
        {CKA_EXTRACTABLE, &falsevalue, sizeof (falsevalue)}
    };
    extern char *optarg;
    extern int optopt;
    
    while ((c = getopt(argc, argv, ":Ps:b:i:l:p:")) != -1) {
        switch (c) {
	case 'P':
	    hide = 0;
	    break;
	case 's':
	    slot = atoi(optarg);
	    break;
        case 'b':
            modulusbits = atoi(optarg);
            break;
        case 'l':
            label = (CK_CHAR *)optarg;
            break;
        case 'p':
            pin = (CK_UTF8CHAR *)optarg;
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
    if ((errflg) || (!modulusbits) || (!label)) {
        fprintf(stderr,
		"usage: genkey [-P] [-s slot] -b keysize -l label [-p pin]\n");
        exit(2);
    }
    
    search_template[0].pValue = label;
    search_template[0].ulValueLen = strlen((char *)label);
    publickey_template[0].pValue = label;
    publickey_template[0].ulValueLen = strlen((char *)label);
    privatekey_template[0].pValue = label;
    privatekey_template[0].ulValueLen = strlen((char *)label);

    /* Set public exponent to 65537 */
    public_exponent[0] = 0x01;
    public_exponent[1] = 0x00;
    public_exponent[2] = 0x01;

    /* Set up mechanism for generating key pair */
    genmech.mechanism = CKM_RSA_PKCS_KEY_PAIR_GEN;
    genmech.pParameter = NULL_PTR;
    genmech.ulParameterLen = 0;

    /* Initialize the CRYPTOKI library */
    rv = C_Initialize(NULL_PTR);

    if (rv != CKR_OK) {
        fprintf(stderr, "C_Initialize: Error = 0x%.8X\n", rv);
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
   
    /* check if a key with the same id already exists */
    rv = C_FindObjectsInit(hSession, search_template, 1); 
    if (rv != CKR_OK) {
        fprintf(stderr, "C_FindObjectsInit: Error = 0x%.8X\n", rv);
        error = 1;
        goto exit_session;
    }
    rv = C_FindObjects(hSession, &privatekey, 1, &ulObjectCount);
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
    
    /* Set attributes if the key is not to be hidden */
    if (!hide) {
        privatekey_template[4].pValue = &falsevalue;
	privatekey_template[5].pValue = &truevalue;
    }

    /* Generate Key pair for signing/verifying */
    rv = C_GenerateKeyPair(hSession, &genmech, publickey_template,
			   (sizeof (publickey_template) /
			    sizeof (CK_ATTRIBUTE)),
			   privatekey_template,
			   (sizeof (privatekey_template) /
			    sizeof (CK_ATTRIBUTE)),
			   &publickey, &privatekey);
        
    if (rv != CKR_OK) {
        fprintf(stderr, "C_GenerateKeyPair: Error = 0x%.8X\n", rv);
        error = 1;
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
