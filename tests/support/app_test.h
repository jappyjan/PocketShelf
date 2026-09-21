#ifndef POCKETSHELF_TESTS_SUPPORT_APP_TEST_H
#define POCKETSHELF_TESTS_SUPPORT_APP_TEST_H
#include "app/app.h"
#include "app/browser.h"
#include "app/jobs.h"
#include "app/keyboard.h"
#include "app/state.h"
#include "core/book.h"
#include "core/text.h"
#include "platform/library.h"
#include "platform/runtime.h"
#include "support/inkview_mock.h"
#include "support/library_fixture.h"
#include "ui/catalog.h"
void test_drain_browse(App *);
void test_complete_keyboard(App *, const char *);
void test_tap_back(App *);
void test_signin(App *);
void test_books(App *);
void test_browse(App *);

#endif
