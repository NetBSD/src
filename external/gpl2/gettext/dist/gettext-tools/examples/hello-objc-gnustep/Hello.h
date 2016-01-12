/* Example for use of GNU gettext.
   Copyright (C) 2003 Free Software Foundation, Inc.
   This file is in the public domain.

   Interface of the Hello class.  */

#include <AppKit/AppKit.h>

@interface Hello : NSObject
{
  NSWindow *window;

  NSTextField *label1;
  NSTextField *label2;

  id okButton;
}

- (id)init;
- (void)dealloc;

- (void)makeKeyAndOrderFront;

- (void)done;

@end

@interface Hello (UIBuilder)

- (void)createUI;

@end
