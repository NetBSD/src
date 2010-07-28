#include <unistd.h>

class Test {
public:
	Test()
	{
		static const char msg[] = "constructor executed\n";
		write(STDOUT_FILENO, msg, sizeof(msg) - 1);
	}
	~Test()
	{
		static const char msg[] = "destructor executed\n";
		write(STDOUT_FILENO, msg, sizeof(msg) - 1);
	}
};

Test test;
