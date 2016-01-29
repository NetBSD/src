#define linker_warning(x, msg) \
	static const char __warn_##x[] \
	__attribute__((used, section(".gnu.warning." #x))) \
	= msg

void bar (void) {} 
linker_warning(bar, "Bad bar"); 
