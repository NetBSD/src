#include <stdio.h>
#include <sl.h>

struct dump_options {
    char* config_file_string;
    char* realm_string;
};
int iprop_dump(struct dump_options*, int, char **);
struct truncate_options {
    char* config_file_string;
    char* realm_string;
};
int iprop_truncate(struct truncate_options*, int, char **);
struct replay_options {
    int start_version_integer;
    int end_version_integer;
    char* config_file_string;
    char* realm_string;
};
int iprop_replay(struct replay_options*, int, char **);
struct last_version_options {
    char* config_file_string;
    char* realm_string;
};
int last_version(struct last_version_options*, int, char **);
int help(void*, int, char **);
extern SL_cmd commands[];
