#include <sys/param.h>
#include <uvm/uvm_extern.h>
extern struct pmap kernel_pmap_;
struct pmap *const kernel_pmap_ptr = &kernel_pmap_;
