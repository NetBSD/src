#ifndef CDKSCREEN_H
#define CDK_SCREEN_H

extern "C"
{
#include <cdk.h>
}

class CDKScreen 
{
  // The window which curses uses.
  WINDOW *cursesWin;
  // The CDKSCREEN struct assigned to this object.
  CDKSCREEN *cdkscreen;
 public:
  // Constructor.
  CDKScreen();
  // Deconstructor.
  ~CDKScreen();
  // Return a pointer to the CDKScreen structure.
  CDKSCREEN *screen(void);
  // Refresh the screen.
  // Note, this function is renamed to avoid clashing with the refresh() macro.
  void refreshscr(void);
  // Erase, but don't destroy, all widgets.
  // Note, this function is renamed to avoid clashing with the erase() macro.
  void erasescr(void);
};

#endif
