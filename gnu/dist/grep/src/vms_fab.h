/*
   This file includes the setup for the file access block for VMS.
   Written by Phillip C. Brisco 8/98.
 */

#include <rms.h>
#include <ssdef.h>
#include <stddef.h>

struct FAB fab;
struct NAM nam;

int length_of_fna_buffer;
int fab_stat;
char expanded_name[NAM$C_MAXRSS];
char fna_buffer[NAM$C_MAXRSS];
char result_name[NAM$C_MAXRSS];
char final_name[NAM$C_MAXRSS];
int max_file_path_size = NAM$C_MAXRSS;
char *arr_ptr[32767]:
