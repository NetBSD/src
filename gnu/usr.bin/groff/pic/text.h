// -*- C++ -*-
//	$Id: text.h,v 1.2 1993/08/02 17:44:50 mycroft Exp $

enum hadjustment {
  CENTER_ADJUST,
  LEFT_ADJUST,
  RIGHT_ADJUST
  };

enum vadjustment {
  NONE_ADJUST,
  ABOVE_ADJUST,
  BELOW_ADJUST
  };

struct adjustment {
  hadjustment h;
  vadjustment v;
};

struct text_piece {
  char *text;
  adjustment adj;
  const char *filename;
  int lineno;

  text_piece();
  ~text_piece();
};
