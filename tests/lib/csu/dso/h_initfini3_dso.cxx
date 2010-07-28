#include <unistd.h>

class Test2 {
public:
	Test2()
	{
		static const char msg[] = "constructor2 executed\n";
		write(STDOUT_FILENO, msg, sizeof(msg) - 1);
	}
	~Test2()
	{
		static const char msg[] = "destructor2 executed\n";
		write(STDOUT_FILENO, msg, sizeof(msg) - 1);
	}
};

Test2 test2;
