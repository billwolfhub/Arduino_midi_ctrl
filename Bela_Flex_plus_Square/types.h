#pragma once
#include <Trill.h>

struct ChannelConfig {
  uint8_t address;
  int midiCC;
  bool doubleTapMute;
};

struct SquareConfig {
  uint8_t address;
  int ccX;
  int ccY;
};

struct TrillChannel {
  Trill trill;
  int midiCC;
  bool doubleTapMute;
  int lastVal          = -1;
  int savedVal         = 64;
  bool muted           = false;
  bool wasTouching     = false;
  unsigned long lastTapTime    = 0;
  unsigned long touchStartTime = 0;
};

struct SquareChannel {
  Trill trill;
  int ccX;
  int ccY;
  int lastX = -1;
  int lastY = -1;
  bool active = false;
};