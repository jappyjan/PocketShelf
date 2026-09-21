#ifndef POCKETSHELF_TESTS_SUPPORT_INKVIEW_MOCK_H
#define POCKETSHELF_TESTS_SUPPORT_INKVIEW_MOCK_H
#include "ui/widgets.h"
#include <pthread.h>
typedef struct {
  int handoff_count, handoff_pending, in_keyboard_callback, nested_keyboard,
      keyboard_calls, keyboard_flags, header_identity, close_calls,
      active_color, error_heading_color, full_updates, partial_updates,
      initialization_flags;
  pthread_t ui_thread;
  iv_keyboardhandler callback;
  char *keyboard_buffer;
  void (*pending_timer)(void);
  char drawn[16384];
} MockInkView;
extern MockInkView mock_ui;

#endif
