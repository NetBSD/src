/*	$NetBSD: set_key_id.c,v 1.1.1.1.2.2 2009/05/13 18:50:30 jym Exp $	*/

/* set_key_id [-s slot] [-p $pin] -n $keytag {-i $id | -l $label} */

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

int
main(int argc, char *argv[])
{
    CK_RV rv;
    CK_SLOT_ID slot = 0;
    CK_SESSION_HANDLE hSession;
    CK_UTF8CHAR *pin = NULL;
    CK_BYTE old_id[2], new_id[2];
    CK_OBJECT_HANDLE akey;
    int error = 0;
    int i = 0;
    int c, errflg = 0;
    char *label = NULL;
    CK_ULONG ulObjectCount;
    int oid = 0, nid = 0;
    CK_ATTRIBUTE search_template[] = {
	{CKA_ID, &old_id, sizeof(old_id)}
    };
    extern char *optarg;
    extern int optopt;

    while ((c = getopt(argc, argv, ":s:i:n:l:p:")) != -1) {
        switch (c) {
	case 's':
	    slot = atoi(optarg);
	    break;
        case 'i':
            oid = atoi(optarg);
	    oid &= 0xffff;
	    old_id[0] = (oid >> 8) & 0xff;
	    old_id[1] = oid & 0xff;
            break;
        case 'n':
            nid = atoi(optarg);
	    nid &= 0xffff;
            new_id[0] = (nid >> 8) & 0xff;
            new_id[1] = nid & 0xff;
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
        case '?':
	default:
            fprintf(stderr, "Unrecognised option: -%c\n", optopt);
            errflg++;
        }
    }
    if ((errflg) || (!nid) || ((!oid) && (!label))) {
        fprintf(stderr,
		"usage: set_key_id [-s slot] [-p pin] -n new_id "
		"{ -i old_id | -l label }\n");
        exit(1);
    }
    if (!label)
	printf("old %i new %i\n", oid, nid);
    else {
	printf("label %s new %i\n", label, nid);
	search_template[0].type = CKA_LABEL;
	search_template[0].pValue = label;
	search_template[0].ulValueLen = strlen(label);
    }

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

    rv = C_FindObjectsInit(hSession, search_template, 1); 
    if (rv != CKR_OK) {
        fprintf(stderr, "C_FindObjectsInit: Error = 0x%.8X\n", rv);
        error = 1;
        goto exit_session;
    }
    
    ulObjectCount = 1;
    while(ulObjectCount) {
        rv = C_FindObjects(hSession, &akey, 1, &ulObjectCount);
        if (rv != CKR_OK) {
            fprintf(stderr, "C_FindObjects: Error = 0x%.8X\n", rv);
            error = 1;
            goto exit_search;
        } else if (ulObjectCount) {       
            /* Set update template. */
            CK_ATTRIBUTE new_template[] = {
                {CKA_ID, &new_id, sizeof(new_id)}
            };

            rv = C_SetAttributeValue(hSession, akey, new_template, 1);
            if (rv != CKR_OK) {
                fprintf(stderr, "C_SetAttributeValue: rv = 0x%.8X\n", rv);
                error = 1;
            }
        }
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
