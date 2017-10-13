#ifndef __TXLIB_H
#define __TXLIB_H 1

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>


// if the window gets larger than this value... boom!
#define MAX_ALLOCATED		160
#define LINE_WIDTH			80
#define MAX_LINES 			LINES
#define MAX_WINDOWS			10
#define DIR_DOWN				0
#define DIR_UP					1
#define DIR_FIRST				2
#define DIR_LAST				3
#define SELECT_NEXT			0
#define SELECT_PREV			1
#define SELECT_FIRST		3
#define SELECT_LAST			4
#define SELECT_N0				1000
#define SELECT_N1				1001
#define SELECT_N2				1002
#define SELECT_N3				1003

#define FG_BLACK           0x00
#define FG_RED             0x01
#define FG_GREEN           0x02
#define FG_YELLOW          0x03
#define FG_BLUE            0x04
#define FG_MAGENTA         0x05
#define FG_CYAN            0x06
#define FG_WHITE					 0x07

#define DEF_COLOR						FG_BLACK

#define VIS_HIDE						0
#define VIS_SHOW						1
#define VIS_SWITCH					2

#define CHAR_ATTRIBUTES(c,a)						(attributes (c, a) + 1)
#define GET_CHAR_ATTRIBUTES(win,x,y)		(win->attributes[x][y] - 1)
#define ATTR_INVALID										(-1)

#define WIN_SELECTED(sc)				(sc->wins[sc->wselected])

#define BLACK_AND_WHITE				0
#define TRANSPARENT_BG				1
#define WHITE_BG							2


#ifdef GDEBUG
# ifdef WINDOWS
#  include <ncurses/curses.h>
# else
#  include <ncurses.h>
# endif
//#include <termios.h>

# define ATTRIB_BOLD					A_BOLD
# define ATTRIB_DIM						A_DIM
# define ATTRIB_INVERT				A_REVERSE

# ifdef NO_A_STANDOUT
#  define ATTRIB_STANDOUT			A_REVERSE
# else
#  define ATTRIB_STANDOUT			A_STANDOUT
# endif
#endif


typedef struct WindowTag
{
	int x, y;
	int nlines;
	char *lines[MAX_ALLOCATED];
	int *attributes[MAX_ALLOCATED];
	char title[LINE_WIDTH + 1];
	
	int hidden;
	int drawn_x, drawn_y;
	
	int fixed_size;
	int title_attributes;
	int default_attributes;
	int no_title;
	int always_visible;


	void (*scroll_callback) (struct WindowTag *win, int nlines, int direction, void *data);
	void *scroll_data;
	
	void (*size_change_callback) (struct WindowTag *win, int new_size, void *data);
	void *size_change_data;

	int (*keypress_callback) (struct WindowTag *win, int key, int modifiers, void *data);
	void *keypress_data;
	
	void (*refresh_callback) (struct WindowTag *win, void *data);
	void *refresh_data;
	
	void (*enter_callback) (struct WindowTag *win, void *data);
	void *enter_data;
} Window;


typedef struct ScreenTag
{
	Window *wins[MAX_WINDOWS];
	int nwindows;
	int wselected;
	
	int (*keypress_callback) (struct ScreenTag *screen, int key, int modifiers, void *data);
	void *keypress_data;

	int (*unhandled_keypress_callback) (struct ScreenTag *screen, int key, int modifiers, void *data);
	void *unhandled_keypress_data;
} Screen;




int attributes (int color, int attribs);
void window_set_title_attributes (Window *win, int color, int attr);
void cursor_goto (int x, int y);
Window *window_create (int x, int y, int nlines, char *title);
void window_destroy (Window *win);
int window_draw_positioned (Window *win, int x, int y);
int window_draw (Window *win);
int window_printf (Window *win, int xrel, int yrel, char *format, ...);
int window_title_printf (Window *win, int xrel, char *format, ...);
int window_cprintf (Window *win, int xrel, int yrel, int color, char *format, ...);
void window_clear_line (Window *win, int line);
void window_roll (Window *win, int lines, int direction);
void screen_init (Screen *screen, int color);
void screen_cleanup (Screen *screen);
void screen_goto_selected (Screen *screen);
void screen_refresh (Screen *screen);
void screen_add_window (Screen *screen, Window *win);
void screen_select_window (Screen *screen, int dir);
void window_add_scroll_callback (Window *win, void *callback, void *data);
void window_add_size_change_callback (Window *win, void *callback, void *data);
void window_scroll (Window *win, int lines, int direction);
void window_line_set_attribs (Window *win, int line, int x1, int x2, int color, int attr);


void window_expand (Window *win, int n);
void window_collapse (Window *win, int n);
void screen_expand_window (Screen *screen, int n);
void screen_collapse_window (Screen *screen, int n);
void screen_redraw (Screen *screen);
void screen_redraw_window (Screen *screen, Window *win);
void window_add_keypress_callback (Window *win, void *callback, void *data);
int window_handle_keypress (Window *win, int key, int modifiers);
void window_add_refresh_callback (Window *win, void *callback, void *data);
void screen_add_keypress_callback (Screen *screen, void *callback, void *data);
void screen_add_unhandled_keypress_callback (Screen *screen, void *callback, void *data);
int screen_handle_keypress (Screen *screen, int key, int modifiers);

void window_enter (Window *win);
void window_add_enter_callback (Window *win, void *callback, void *data);
void screen_set_cursor_relative (Screen *screen, Window *win, int x, int y);

void window_refresh (Window *win);
int screen_count_empty_lines (Screen *screen);
int window_is_resizable (Window *win, int size_restriction);
int screen_find_resizable (Screen *screen, int n, int size_restriction);
void screen_expand (Screen *screen, int n);
void screen_collapse (Screen *screen, int n);

void screen_hide_window (Screen *screen, int sw, int n);
void screen_hide_window_selected (Screen *screen, int sw);

int screen_handle_input (Screen *screen, int key);

void window_clear (Window *win);
void window_title_clear (Window *win, int a, int b);
void window_set_title (Window *win, char *title);

int screen_getkey (Screen *screen);

#endif // __TXLIB_H 1
