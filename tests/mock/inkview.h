#ifndef MOCK_INKVIEW_H
#define MOCK_INKVIEW_H
typedef struct {
  int size;
} ifont;
typedef void (*iv_keyboardhandler)(char *);
typedef struct {
  int width, height;
} ibitmap;
#define kFmtRGB24 1
#define STRETCH 4096
ibitmap *LoadImageToFormat(const char *, int);
void DrawBitmapRect(int, int, int, int, const ibitmap *, int);
int StringWidth(const char *);
#define BLACK 0
#define TASK_MAKEACTIVE (1 << 7)
#define TASK_FB_RGB24 (1 << 20)
typedef struct {
  int depth;
} icanvas;
icanvas *GetCanvas(void);
void InitInkview(int);
void FillArea(int, int, int, int, int);
#define ALIGN_LEFT 1
#define ALIGN_RIGHT 4
#define VALIGN_TOP 16
#define KBD_PASSWORD 0x100
#define EVT_INIT 21
#define EVT_EXIT 22
#define EVT_SHOW 23
#define EVT_KEYPRESS 25
#define EVT_POINTERUP 29
#define IV_KEY_BACK 0x1b
#define IV_KEY_NEXT 0x19
#define IV_KEY_PREV 0x18
void SetFont(const ifont *, int);
char *DrawTextRect(int, int, int, int, const char *, int);
void DrawRect(int, int, int, int, int);
void DrawLine(int, int, int, int, int);
void ClearScreen(void);
void FullUpdate(void);
int ScreenWidth(void);
int ScreenHeight(void);
void CloseApp(void);
void OpenKeyboard(const char *, char *, int, int, iv_keyboardhandler);
void SetHardTimer(const char *, void (*)(void), int);
void ClearTimer(void (*)(void));
int NetConnect2(const char *, int);
void BookPreparing(const char *);
void BookReady(const char *);
int OpenBook(const char *, const char *, int);
ifont *OpenFont(const char *, int, int);
void CloseFont(ifont *);
void InkViewMain(int (*)(int, int, int));
#endif
