#include <stdio.h>
#include <sl.h>

struct cms_create_sd_options {
    struct getarg_strings certificate_strings;
    char* signer_string;
    struct getarg_strings anchors_strings;
    struct getarg_strings pool_strings;
    struct getarg_strings pass_strings;
    struct getarg_strings peer_alg_strings;
    char* content_type_string;
    int content_info_flag;
    int pem_flag;
    int detached_signature_flag;
    int id_by_name_flag;
};
int cms_create_sd(struct cms_create_sd_options*, int, char **);
struct cms_verify_sd_options {
    struct getarg_strings anchors_strings;
    struct getarg_strings certificate_strings;
    struct getarg_strings pass_strings;
    int missing_revoke_flag;
    int content_info_flag;
    char* signed_content_string;
};
int cms_verify_sd(struct cms_verify_sd_options*, int, char **);
struct cms_unenvelope_options {
    struct getarg_strings certificate_strings;
    struct getarg_strings pass_strings;
    int content_info_flag;
};
int cms_unenvelope(struct cms_unenvelope_options*, int, char **);
struct cms_envelope_options {
    struct getarg_strings certificate_strings;
    struct getarg_strings pass_strings;
    char* encryption_type_string;
    char* content_type_string;
    int content_info_flag;
};
int cms_create_enveloped(struct cms_envelope_options*, int, char **);
struct verify_options {
    struct getarg_strings pass_strings;
    int allow_proxy_certificate_flag;
    int missing_revoke_flag;
    char* time_string;
    int verbose_flag;
    int max_depth_integer;
    char* hostname_string;
};
int pcert_verify(struct verify_options*, int, char **);
struct print_options {
    struct getarg_strings pass_strings;
    int content_flag;
    int info_flag;
};
int pcert_print(struct print_options*, int, char **);
struct validate_options {
    struct getarg_strings pass_strings;
};
int pcert_validate(struct validate_options*, int, char **);
struct certificate_copy_options {
    struct getarg_strings in_pass_strings;
    char* out_pass_string;
};
int certificate_copy(struct certificate_copy_options*, int, char **);
struct ocsp_fetch_options {
    struct getarg_strings pass_strings;
    char* sign_string;
    char* url_path_string;
    int nonce_flag;
    struct getarg_strings pool_strings;
};
int ocsp_fetch(struct ocsp_fetch_options*, int, char **);
struct ocsp_verify_options {
    char* ocsp_file_string;
};
int ocsp_verify(struct ocsp_verify_options*, int, char **);
struct ocsp_print_options {
    int verbose_flag;
};
int ocsp_print(struct ocsp_print_options*, int, char **);
struct request_create_options {
    char* subject_string;
    struct getarg_strings email_strings;
    struct getarg_strings dnsname_strings;
    char* type_string;
    char* key_string;
    char* generate_key_string;
    int key_bits_integer;
    int verbose_flag;
};
int request_create(struct request_create_options*, int, char **);
struct request_print_options {
    int verbose_flag;
};
int request_print(struct request_print_options*, int, char **);
struct query_options {
    int exact_flag;
    int private_key_flag;
    char* friendlyname_string;
    int keyEncipherment_flag;
    int digitalSignature_flag;
    int print_flag;
    struct getarg_strings pass_strings;
};
int query(struct query_options*, int, char **);
int info(void*, int, char **);
int random_data(void*, int, char **);
struct crypto_available_options {
    char* type_string;
};
int crypto_available(struct crypto_available_options*, int, char **);
struct crypto_select_options {
    char* type_string;
    char* certificate_string;
    struct getarg_strings peer_cmstype_strings;
};
int crypto_select(struct crypto_select_options*, int, char **);
struct hex_options {
    int decode_flag;
};
int hxtool_hex(struct hex_options*, int, char **);
struct certificate_sign_options {
    int issue_ca_flag;
    int issue_proxy_flag;
    int domain_controller_flag;
    char* subject_string;
    char* ca_certificate_string;
    int self_signed_flag;
    char* ca_private_key_string;
    char* certificate_string;
    struct getarg_strings type_strings;
    char* lifetime_string;
    char* serial_number_string;
    int path_length_integer;
    struct getarg_strings hostname_strings;
    struct getarg_strings email_strings;
    char* pk_init_principal_string;
    char* ms_upn_string;
    char* jid_string;
    char* req_string;
    char* certificate_private_key_string;
    char* generate_key_string;
    int key_bits_integer;
    char* crl_uri_string;
    char* template_certificate_string;
    char* template_fields_string;
};
int hxtool_ca(struct certificate_sign_options*, int, char **);
struct test_crypto_options {
    struct getarg_strings pass_strings;
    int verbose_flag;
};
int test_crypto(struct test_crypto_options*, int, char **);
struct statistic_print_options {
    int type_integer;
};
int statistic_print(struct statistic_print_options*, int, char **);
struct crl_sign_options {
    char* signer_string;
    struct getarg_strings pass_strings;
    char* crl_file_string;
    char* lifetime_string;
};
int crl_sign(struct crl_sign_options*, int, char **);
int help(void*, int, char **);
extern SL_cmd commands[];
