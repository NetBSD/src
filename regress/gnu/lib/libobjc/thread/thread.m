/* Written by David Wetzel */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <objc/objc.h>
#include <objc/objc-api.h>
#include <objc/Object.h>

static const int debug = 0;

@interface MyClass : Object
{
  char *myName;
}
-(void)setMyName:(const char *)n;
-(const char *)myName;
@end

@implementation MyClass
-(void)setMyName:(const char *)n
{
    size_t len;

    if (myName) {
	    if (strcmp(myName, n) != 0)
		    return;
	    objc_free(myName);
    }
    len = strlen(n) + 1;
    myName = objc_malloc(len);
    strcpy(myName, n);
}
-(char *)myName
{
    return myName;
}
-(void)start
{
	if (debug)
		printf("detached thread started!\n");
}
@end

static void
becomeMultiThreaded(void)
{
	if (debug)
		printf("becoming multithreaded!\n");
}

int
main(int argc, char *argv[])
{
	id o = [MyClass new];
	const char *c;
	objc_thread_callback cb;
	objc_thread_t rv;

	[o setMyName:"thread"];
	c = [o myName];

	if (debug)
		printf("Testing: %s\n",c);

	cb = objc_set_thread_callback(becomeMultiThreaded);
	if (debug)
		printf("Old Callback: %p\n",cb);

	rv = objc_thread_detach(@selector(start), o, nil);
	if (debug)
		printf("detach value: %p\n",rv);
	assert(rv != NULL);

	return 0;
}
