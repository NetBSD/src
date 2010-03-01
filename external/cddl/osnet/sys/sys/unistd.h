
#include_next <sys/unistd.h>

#define _PC_ACL_ENABLED         20
#define _PC_SATTR_ENABLED       23
#define _PC_SATTR_EXISTS        24
#define _PC_ACCESS_FILTERING    25
/* UNIX 08 names */
#define _PC_TIMESTAMP_RESOLUTION 26


/*
 * The value of 0 is returned when
 * ACL's are not supported
 */
#define _ACL_ACLENT_ENABLED     0x1
#define _ACL_ACE_ENABLED        0x2
