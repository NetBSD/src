#include <stdio.h>
#include <sl.h>

struct add_options {
    char* principal_string;
    int kvno_integer;
    char* enctype_string;
    char* password_string;
    int salt_flag;
    int random_flag;
    int hex_flag;
};
int kt_add(struct add_options*, int, char **);
struct change_options {
    char* realm_string;
    char* admin_server_string;
    int server_port_integer;
};
int kt_change(struct change_options*, int, char **);
int kt_copy(void*, int, char **);
struct get_options {
    char* principal_string;
    struct getarg_strings enctypes_strings;
    char* realm_string;
    char* admin_server_string;
    int server_port_integer;
};
int kt_get(struct get_options*, int, char **);
struct list_options {
    int keys_flag;
    int timestamp_flag;
};
int kt_list(struct list_options*, int, char **);
struct purge_options {
    char* age_string;
};
int kt_purge(struct purge_options*, int, char **);
struct remove_options {
    char* principal_string;
    int kvno_integer;
    char* enctype_string;
};
int kt_remove(struct remove_options*, int, char **);
int kt_rename(void*, int, char **);
struct srvconvert_options {
    char* srvtab_string;
};
int srvconv(struct srvconvert_options*, int, char **);
struct srvcreate_options {
    char* srvtab_string;
};
int srvcreate(struct srvcreate_options*, int, char **);
int help(void*, int, char **);
extern SL_cmd commands[];
