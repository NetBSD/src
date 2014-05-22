// RUN: %clang_cc1  -fsyntax-only -Wselector -verify -Wno-objc-root-class %s
// rdar://8851684

@interface Foo
- (void) foo;
- (void) bar;
@end

@implementation Foo
- (void) bar
{
}

- (void) foo
{
  SEL a,b,c;
  a = @selector(b1ar);  // expected-warning {{no method with selector 'b1ar' is implemented in this translation unit}}
  b = @selector(bar);
}
@end

@interface I
- length;
@end

SEL func()
{
    return  @selector(length);  // expected-warning {{no method with selector 'length' is implemented in this translation unit}}
}

// rdar://9545564
@class MSPauseManager;

@protocol MSPauseManagerDelegate 
@optional
- (void)pauseManagerDidPause:(MSPauseManager *)manager;
- (int)respondsToSelector:(SEL)aSelector;
@end

@interface MSPauseManager
{
  id<MSPauseManagerDelegate> _delegate;
}
@end


@implementation MSPauseManager
- (id) Meth {
  if ([_delegate respondsToSelector:@selector(pauseManagerDidPause:)])
    return 0;
  return 0;
}
@end

// rdar://12938616
@class NSXPCConnection;

@interface NSObject
@end

@interface INTF : NSObject
{
  NSXPCConnection *cnx; // Comes in as a parameter.
}
- (void) Meth;
@end

extern SEL MySelector(SEL s);

@implementation INTF
- (void) Meth {
  if( [cnx respondsToSelector:MySelector(@selector( _setQueue: ))] ) // expected-warning {{no method with selector '_setQueue:' is implemented in this translation unit}}
  {
  }

  if( [cnx respondsToSelector:@selector( _setQueueXX: )] ) // No warning here.
  {
  }
  if( [cnx respondsToSelector:(@selector( _setQueueXX: ))] ) // No warning here.
  {
  }
}
@end

// rdar://14007194
@interface UxTechTest : NSObject
- (int) invalidate : (id)Arg;
+ (int) C_invalidate : (int)arg;
@end

@interface UxTechTest(CAT)
- (char) invalidate : (int)arg;
+ (int) C_invalidate : (char)arg;
@end

@interface NSPort : NSObject
- (double) invalidate : (void*)Arg1;
+ (int) C_invalidate : (id*)arg;
@end


@interface USEText : NSPort
- (int) invalidate : (int)arg;
@end

@implementation USEText
- (int) invalidate :(int) arg { return 0; }
@end

@interface USETextSub : USEText
- (int) invalidate : (id)arg;
@end
