/* Example for use of GNU gettext.
   Copyright (C) 2003 Free Software Foundation, Inc.
   This file is in the public domain.

   Interface of the AppController class.  */

#include <AppKit/AppKit.h>

@class Hello;

@interface AppController : NSObject
{
  Hello *hello;
}

+ (void)initialize;

- (id)init;
- (void)dealloc;

- (void)awakeFromNib;

- (void)applicationDidFinishLaunching
    :(NSNotification *)notif;

- (BOOL)applicationShouldTerminate:(id)sender;
- (void)applicationWillTerminate:(NSNotification *)notification;

- (BOOL)application:(NSApplication *)application openFile:(NSString *)fileName;

- (void)showPrefPanel:(id)sender;
- (void)showInfoPanel:(id)sender;

- (void)showHelloWindow:(id)sender;

@end
