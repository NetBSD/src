#include <stdio.h>
#include <sl.h>

struct digest_probe_options {
    char* realm_string;
};
int digest_probe(struct digest_probe_options*, int, char **);
struct digest_server_init_options {
    char* type_string;
    char* kerberos_realm_string;
    char* digest_string;
    char* cb_type_string;
    char* cb_value_string;
    char* hostname_string;
    char* realm_string;
};
int digest_server_init(struct digest_server_init_options*, int, char **);
struct digest_server_request_options {
    char* type_string;
    char* kerberos_realm_string;
    char* username_string;
    char* server_nonce_string;
    char* server_identifier_string;
    char* client_nonce_string;
    char* client_response_string;
    char* opaque_string;
    char* authentication_name_string;
    char* realm_string;
    char* method_string;
    char* uri_string;
    char* nounce_count_string;
    char* qop_string;
    char* ccache_string;
};
int digest_server_request(struct digest_server_request_options*, int, char **);
struct digest_client_request_options {
    char* type_string;
    char* username_string;
    char* password_string;
    char* server_nonce_string;
    char* server_identifier_string;
    char* client_nonce_string;
    char* opaque_string;
    char* realm_string;
    char* method_string;
    char* uri_string;
    char* nounce_count_string;
    char* qop_string;
};
int digest_client_request(struct digest_client_request_options*, int, char **);
struct ntlm_server_init_options {
    int version_integer;
    char* kerberos_realm_string;
};
int ntlm_server_init(struct ntlm_server_init_options*, int, char **);
int help(void*, int, char **);
extern SL_cmd commands[];
