
#include <stdlib.h>
#include <string.h>
#include <trousers/tss.h>
#include <trousers/trousers.h>
#include "spi_internal_types.h"
#include <spi_utils.h>
#include <obj.h>
#include "tsplog.h"
#include "daa_parameter.h"

setenv("TCSD_FOREGROUND", "1", 1);

// simulating Tspi_TPM_DAA_JoinInit (spi_daa.c)
TSS_RESULT Tspi_DAA_Join(TSS_HTPM hTPM, int stage, UINT32 inputSize0, BYTE *inputData0, UINT32 inputSize1, BYTE *inputData1, UINT32 *outputSize, BYTE **outputData) {
	TSS_RESULT result;
	TCS_CONTEXT_HANDLE tcsContext;
	TSS_HCONTEXT tspContext;
	TSS_HPOLICY hPolicy;
	TCPA_DIGEST digest;
	TPM_AUTH ownerAuth;
	UINT16 offset = 0;
	BYTE hashblob[1000];

	printf("[%s:%d] obj_tpm_is_connected(hTPM)\n", __FILE__, __LINE__);
	if( (result = obj_tpm_is_connected(  hTPM, &tcsContext)) != TSS_SUCCESS) return result;
	printf("[%s:%d] obj_tpm_get_tsp_context(hTPM)\n", __FILE__, __LINE__);
	if( (result = obj_tpm_get_tsp_context( hTPM, &tspContext)) != TSS_SUCCESS) return result;
	printf("[%s:%d] obj_tpm_get_policy(hTPM)\n", __FILE__, __LINE__);
	if( (result = obj_tpm_get_policy( hTPM, &hPolicy)) != TSS_SUCCESS) return result;

	printf("[%s:%d] Trspi_LoadBlob_UINT32(&offset, TPM_ORD_DAA_Join, hashblob)\n", __FILE__, __LINE__);
	Trspi_LoadBlob_UINT32(&offset, TPM_ORD_DAA_Join, hashblob); // hash TPM_COMMAND_CODE
	printf("[%s:%d] Trspi_Hash(TSS_HASH_SHA1, offset, hashblob, digest.digest)\n",__FILE__, __LINE__);
	Trspi_LoadBlob_BYTE(&offset, stage, hashblob); // hash stage
	printf("[%s:%d] Trspi_LoadBlob_UINT32(&offset, 0, hashblob)\n",__FILE__, __LINE__);
	//TODO old 4
	Trspi_LoadBlob_UINT32(&offset, inputSize0, hashblob); // hash inputSize0
	printf("[%s:%d] Trspi_LoadBlob_UINT32(&offset, 0, hashblob)\n",__FILE__, __LINE__);
	Trspi_LoadBlob( &offset, inputSize0, hashblob, inputData0); // hash inputData0
	//TODO old 1
	Trspi_LoadBlob_UINT32(&offset, inputSize1, hashblob); // hash inputSize1
	printf("[%s:%d] Trspi_LoadBlob_UINT32(&offset, 0, hashblob)\n",__FILE__, __LINE__);
	Trspi_LoadBlob( &offset, inputSize1, hashblob, inputData1); // hash inputData1
	Trspi_Hash(TSS_HASH_SHA1, offset, hashblob, digest.digest);

	if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_DAA_Join,
	     hPolicy, &digest,
	     &ownerAuth)) != TSS_SUCCESS) return result;
	printf("[%s:%d] secret_PerformAuth_OIAP(hTPM, TPM_ORD_DAA_Join ret=%d\n",__FILE__, __LINE__, result);
	// out

    /* step of the following call:
	TCSP_DAAJoin 		tcsd_api/calltcsapi.c (define in spi_utils.h)
	TCSP_DAAJoin_TP 	tcsd_api/tcstp.c (define in	trctp.h)
    */

	printf("[%s:%d] TCSP_DAAJoin(%x,%x,%x,%x,%x,%x,%x)\n",__FILE__, __LINE__,
	       (int)hTPM, 0, inputSize0,(int)inputData0,inputSize1,(int)inputData1,(int)&ownerAuth);
	if ( (result =  TCSP_DaaJoin( tcsContext, hTPM, 0, inputSize0, inputData0, inputSize1, inputData1, &ownerAuth, outputSize, outputData)) != TSS_SUCCESS)
		return result;

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, result, hashblob);
	Trspi_LoadBlob_UINT32(&offset, TPM_ORD_DAA_Join, hashblob);
	Trspi_LoadBlob_UINT32(&offset, *outputSize, hashblob);
	Trspi_LoadBlob(&offset, *outputSize, hashblob, *outputData);
	Trspi_Hash(TSS_HASH_SHA1, offset, hashblob, digest.digest);
	if( (result = obj_policy_validate_auth_oiap( hPolicy, &digest, &ownerAuth)))
	{
		printf("[%s:%d] obj_policy_validate_auth=%d\n",__FILE__, __LINE__, result);
	}
	return result;
}


int main(int argc, char *argv[])
{
    TSS_HCONTEXT hContext;
    TSS_RESULT result;
    TSS_HTPM hTPM;
    TSS_HPOLICY hPolicy;

    // Create Context
    printf("Create Context\n");
    result = Tspi_Context_Create( &hContext );
    if ( result != TSS_SUCCESS )
    {
        fprintf( stderr, "Tspi_Context_Create %d\n", result );
        exit( result );
    }

    // Connect to Context
    printf("\nConnect to the context\n");
    result = Tspi_Context_Connect( hContext, NULL );
    if ( result != TSS_SUCCESS ) goto out_close;

    if( (result = Tspi_Context_GetTpmObject( hContext, &hTPM)) != TSS_SUCCESS)
        goto out_close;

    // Get the correct policy using the TPM ownership PASSWD
    char *szTpmPasswd = "OWN_PWD";
    if( (result = Tspi_GetPolicyObject( hTPM, TSS_POLICY_USAGE, &hPolicy)) != TSS_SUCCESS)
        goto out_close;
    //BUSS
    if( (result = Tspi_Policy_SetSecret( hPolicy, TSS_SECRET_MODE_PLAIN, strlen( szTpmPasswd), szTpmPasswd)) != TSS_SUCCESS)
        goto out_close;
    printf("Tspi_Policy_SetSecret hPolicy received;%d\n", hPolicy);

     //BUSS
    // in
    //int modulus_length = DAA_PARAM_SIZE_MODULUS_GAMMA / 8;
    UINT32 inputSize0 = sizeof(int);
    UINT32 inputSize1 = 0;
    UINT32 outputSize = 0;
    int ia_length = 7;
    BYTE *inputData0 = (BYTE *)(&ia_length);//= (BYTE *)malloc( inputSize0)
    BYTE *inputData1 = NULL;
    BYTE *outputData = NULL;

    if( (result = Tspi_DAA_Join(hTPM, 0, inputSize0, inputData0, inputSize1, inputData1, &outputSize, &outputData)) != TSS_SUCCESS) goto out_close;

    goto out;
out_close:
	printf( "Tspi Error:%d - %s\n", result, err_string( result) );

out:
	printf("ouputSize=%d\n", outputSize);
	if( outputData != NULL) {
		int i;
		printf("outputData(hex  )=[\n");
		for( i=0; i<(int)outputSize; i++) printf("%x ", outputData[i]);
		printf("\n]");
		printf("outputData(ascii)=[\n");
		for( i=0; i<(int)outputSize; i++) printf("%c ", outputData[i]);
		printf("\n]");
	}
    Tspi_Context_FreeMemory( hContext, NULL );
    Tspi_Context_Close( hContext );
    printf("[%s:%d] THE END\n",__FILE__, __LINE__);
    return result;
}
