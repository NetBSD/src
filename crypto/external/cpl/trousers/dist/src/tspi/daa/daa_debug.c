

/********************************************************************************************
*   KEY PAIR WITH PROOF
********************************************************************************************/

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
/*              proof->challenge (BYTE*) */
/*              response (bi_array_ptr) */
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

