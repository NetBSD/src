#include <netinet/in.h>

#define GULM_MAX_CLUSTER_MESSAGE 1600
#define GULM_MAX_CSID_LEN sizeof(struct in6_addr)
#define GULM_MAX_CLUSTER_MEMBER_NAME_LEN 128

extern int init_comms(unsigned short);
extern char *print_csid(const char *);
int get_main_gulm_cluster_fd(void);
int cluster_fd_gulm_callback(struct local_client *fd, char *buf, int len, const char *csid, struct local_client **new_client);
int gulm_cluster_send_message(void *buf, int msglen, const char *csid, const char *errtext);
void get_our_gulm_csid(char *csid);
int gulm_connect_csid(const char *csid, struct local_client **newclient);
