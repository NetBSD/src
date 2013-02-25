
PTQ_HEAD(thread_queue_t, td_thread_st);
PTQ_HEAD(sync_queue_t, td_sync_st);

struct td_proc_st {
	struct td_proc_callbacks_t *cb;
	void *arg;

	caddr_t dbgaddr;
	caddr_t allqaddr;
	caddr_t runqaddr;	
	caddr_t idleqaddr;
	caddr_t suspqaddr;
	caddr_t maxlwpsaddr;
	caddr_t tsdlistaddr;
	caddr_t tsddestaddr;

	caddr_t stacksizeaddr;
	int stacksizelg;
	size_t stacksize;
	vaddr_t stackmask;
	struct reg *regbuf;
	struct fpreg *fpregbuf;

	struct thread_queue_t threads;
};


struct td_thread_st {
	td_proc_t *proc;
	caddr_t addr;
	lwpid_t lwp;
	PTQ_ENTRY(td_thread_st)	list;
};


struct td_sync_st {
	td_proc_t *proc;
	caddr_t addr;
	PTQ_ENTRY(td_sync_st) list;
};

#define READ(proc, addr, buf, size) ((proc)->cb->proc_read((proc)->arg, (addr), (buf), (size)))
#define WRITE(proc, addr, buf, size) ((proc)->cb->proc_write((proc)->arg, (addr), (buf), (size)))
#define LOOKUP(proc, sym, addr) ((proc)->cb->proc_lookup((proc)->arg, (sym), (addr)))
#define REGSIZE(proc, regset, size) ((proc)->cb->proc_regsize((proc)->arg, (regset), (size)))
#define GETREGS(proc, regset, lwp, buf) ((proc)->cb->proc_getregs((proc)->arg, (regset), (lwp), (buf)))
#define SETREGS(proc, regset, lwp, buf) ((proc)->cb->proc_setregs((proc)->arg, (regset), (lwp), (buf)))
