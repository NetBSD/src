
/********************************************************************************************
 *   KEY PAIR WITH PROOF
 ********************************************************************************************/

typedef struct tdKEY_PAIR_WITH_PROOF_internal {
        TSS_DAA_PK_internal *pk;
        DAA_PRIVATE_KEY_internal *private_key;
        TSS_DAA_PK_PROOF_internal *proof;
} KEY_PAIR_WITH_PROOF_internal;

int save_KEY_PAIR_WITH_PROOF(
        FILE *file,
        KEY_PAIR_WITH_PROOF_internal *key_pair_with_proof
);

KEY_PAIR_WITH_PROOF_internal *load_KEY_PAIR_WITH_PROOF(
        FILE *file
);

TSS_DAA_KEY_PAIR *get_TSS_DAA_KEY_PAIR(
        KEY_PAIR_WITH_PROOF_internal *key_pair_with_proof,
        void * (*daa_alloc)(size_t size, TSS_HOBJECT object),
        TSS_HOBJECT param_alloc
);


int save_DAA_PK_internal(
        FILE *file,
        const TSS_DAA_PK_internal *pk_internal
);

TSS_DAA_PK_internal *load_DAA_PK_internal(
        FILE *file
);

int save_DAA_PRIVATE_KEY(
        FILE *file,
        const DAA_PRIVATE_KEY_internal *private_key
);

DAA_PRIVATE_KEY_internal *load_DAA_PRIVATE_KEY(
        FILE *file
);

int save_DAA_PK_PROOF_internal(
        FILE *file,
        TSS_DAA_PK_PROOF_internal *pk_internal
);

TSS_DAA_PK_PROOF_internal *load_DAA_PK_PROOF_internal(
        FILE *file
);

TSS_DAA_CRED_ISSUER *load_TSS_DAA_CRED_ISSUER( FILE *file);

int save_TSS_DAA_CRED_ISSUER( FILE *file, TSS_DAA_CRED_ISSUER *credential);

TSS_DAA_CREDENTIAL *load_TSS_DAA_CREDENTIAL( FILE *file);

int save_TSS_DAA_CREDENTIAL(
        FILE *file,
        TSS_DAA_CREDENTIAL *credential
);


