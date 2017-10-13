/*
 *  gcube
 *  Copyright (c) 2004 monk
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 *
 *      
 *         
 */

#include "txlib.h"

#define WIN_SELECTED(sc)				(sc->wins[sc->wselected])

#define USE_NCURSES		!FALSE

// library dependant functions
#if USE_NCURSES

#define COLOR_ATTRIB(X)					(COLOR_PAIR (X))

void cursor_goto (int x, int y)
{
	move (y, x);
}


void attributes_select (int attributes)
{
	attrset (attributes);
}


void window_print (char *string)
{
	addstr (string);
}


void window_print_a (char *string, int attributes)
{
	attron (attributes);
	window_print (string);
	attroff (attributes);
}


int textmode_lib_init (void)
{
//	newterm (NULL, stdout, stdin);

	initscr ();
	cbreak ();
	noecho ();
	nonl ();
	keypad (stdscr, TRUE);

	return TRUE;
}


void textmode_lib_cleanup (void)
{
	endwin ();
	fflush (stdout);
	setvbuf (stdout, NULL, _IOLBF, BUFSIZ);
}


void textmode_screen_refresh (void)
{
	refresh ();
}


void textmode_screen_clear (void)
{
	clear ();
}


int colors_init (int color)
{
	int bg;
		

	if (color == TRANSPARENT_BG)
		bg = -1;
	else
		bg = COLOR_WHITE;
		
	start_color ();
	use_default_colors ();
	init_pair (FG_BLACK, COLOR_BLACK, bg);
	init_pair (FG_RED, COLOR_RED, bg);
	init_pair (FG_GREEN, COLOR_GREEN, bg);
	init_pair (FG_YELLOW, COLOR_YELLOW, bg);
	init_pair (FG_BLUE, COLOR_BLUE, bg);
	init_pair (FG_MAGENTA, COLOR_MAGENTA, bg);
	init_pair (FG_CYAN, COLOR_CYAN, bg);
		
	if (color == TRANSPARENT_BG)
	{
		init_pair (FG_WHITE, COLOR_WHITE, bg);
		bkgdset (' ' | COLOR_PAIR (FG_BLACK));
	}
	else
	{
		init_pair (FG_WHITE, COLOR_BLACK, bg);
		bkgdset (' ' | COLOR_PAIR (FG_WHITE));
		assume_default_colors (FG_WHITE, FG_WHITE);
	}
	
	return TRUE;
}


int screen_getkey (Screen *screen)
{
	return getch ();
}


int screen_getkeyhit (Screen *screen)
{
	int key;


	nodelay (stdscr, TRUE);
	key = getch ();
	nodelay (stdscr, FALSE);
	return key;
}


int attributes (int color, int attribs)
{
	return COLOR_ATTRIB (color) | attribs;
}

#else
#include <slang.h>

#define LINES							SLtt_Screen_Rows
#define COLOR_ATTRIB(X) 	X

void cursor_goto (int x, int y)
{
	SLsmg_gotorc (y, x);
}


void attributes_select (int attributes)
{
	SLsmg_set_color (attributes);
}


void window_print (char *string)
{
	SLsmg_write_string (string);
}


void window_print_a (char *string, int attributes)
{
	SLsmg_set_color (attributes);
	window_print (string);
}


int textmode_lib_init (void)
{
	SLtt_get_terminfo ();
	SLkp_init ();
	SLang_init_tty (-1, 0, 1);
	SLsmg_init_smg ();

	return TRUE;
}


void textmode_lib_cleanup (void)
{
	SLang_reset_tty ();
	SLsmg_reset_smg ();
}


void textmode_screen_refresh (void)
{
	SLsmg_refresh ();
}


void textmode_screen_clear (void)
{
	SLsmg_cls ();
}


int colors_init (int color)
{
	char *bg;
		

	if (!SLtt_Use_Ansi_Colors)
		return FALSE;

	if (color == TRANSPARENT_BG)
		bg = "default";
	else
		bg = "white";
		
	SLtt_set_color (FG_BLACK, NULL, "black", bg);
	SLtt_set_color (FG_RED, NULL, "red", bg);
	SLtt_set_color (FG_GREEN, NULL, "green", bg);
	SLtt_set_color (FG_YELLOW, NULL, "yellow", bg);
	SLtt_set_color (FG_BLUE, NULL, "blue", bg);
	SLtt_set_color (FG_MAGENTA, NULL, "magenta", bg);
	SLtt_set_color (FG_CYAN, NULL, "cyan", bg);
		
	if (color == TRANSPARENT_BG)
		SLtt_set_color (FG_WHITE, NULL, "white", bg);
	else
		SLtt_set_color (FG_WHITE, NULL, "black", bg);
	
	return TRUE;
}


int screen_getkey (Screen *screen)
{
	return SLang_getkey ();
}


int attributes (int color, int attribs)
{
	return COLOR_ATTRIB (color);
}

#endif // USE_NCURSES
//////////////


void window_set_title_attributes (Window *win, int color, int attr)
{
	win->title_attributes = COLOR_ATTRIB (color);
}


Window *window_create (int x, int y, int nlines, char *title)
{
	Window *win;
	int i;


	if (x >= LINE_WIDTH || y >= MAX_LINES)
		return NULL;
	
	
	win = calloc (1, sizeof (Window));
	win->x = x;
	win->y = y;
	win->nlines = (nlines > MAX_LINES) ? (MAX_LINES) : nlines;

	memset (win->title, ' ', LINE_WIDTH);
	win->title[LINE_WIDTH] = '\0';
	strncpy (win->title, title, strlen (title));

	window_set_title_attributes (win, DEF_COLOR, 0);
	win->default_attributes = attributes (DEF_COLOR, 0);

	for (i = 0; i < MAX_ALLOCATED; i++)
	{
		win->lines[i] = malloc (LINE_WIDTH + 1);
		memset (win->lines[i], ' ', LINE_WIDTH);
		win->lines[i][LINE_WIDTH] = '\0';
		
		win->attributes[i] = malloc (LINE_WIDTH * sizeof (int));
		memset (win->attributes[i], win->default_attributes, LINE_WIDTH * sizeof (int));
	}
	
	return win;
}


void window_clear (Window *win)
{
	int i;


	for (i = 0; i < MAX_ALLOCATED; i++)
	{
		memset (win->lines[i], ' ', LINE_WIDTH);
		win->lines[i][LINE_WIDTH] = '\0';
		
		memset (win->attributes[i], win->default_attributes, LINE_WIDTH * sizeof (int));
	}
}


void window_destroy (Window *win)
{
	int i;


	for (i = 0; i < MAX_ALLOCATED; i++)
	{
		free (win->lines[i]);
		free (win->attributes[i]);
	}
	
	free (win);
}


// return number of lines drawn
int window_draw_positioned (Window *win, int x, int y)
{
	int i, j, last_j = 0;
	char buff[LINE_WIDTH + 1] = {0};
	int attr = 0;


	cursor_goto (x, y);

	if (!win->no_title)
	{
		window_print_a (win->title, win->title_attributes);
		y++;
	}

	if (win->hidden)
		return 1;

	attr = GET_CHAR_ATTRIBUTES (win, 0, 0);
	if (attr != ATTR_INVALID)
		attributes_select (attr);
	for (i = 0; i < win->nlines; i++)
	{
		cursor_goto (x, y + i);

		j = 0;
		while (j < LINE_WIDTH)
		{
			last_j = j;
			for (j = j; j < LINE_WIDTH; j++)
				if (attr != GET_CHAR_ATTRIBUTES (win, i, j))
				{
					attr = GET_CHAR_ATTRIBUTES (win, i, j);
					break;
				}

			memset (buff, 0, sizeof (buff));
			strncpy (buff, win->lines[i] + last_j, j - last_j);
			window_print (buff);
			if (attr != ATTR_INVALID)
				attributes_select (attr);
		}
	}
	attributes_select (0);

	return win->nlines + 1;
}


int window_draw (Window *win)
{
	return window_draw_positioned (win, win->x, win->y);
}


int window_printf (Window *win, int xrel, int yrel, char *format, ...)
{
	va_list ap;
	char buff[LINE_WIDTH + 1];
	int len, max, l;


	if (!win || yrel >= win->nlines || xrel >= LINE_WIDTH)
		return 0;

	va_start (ap, format);
	vsnprintf (buff, sizeof (buff) - 1, format, ap);
	va_end (ap);
	
	len = strlen (buff);
	max = LINE_WIDTH - xrel;
	l = (len > max) ? max : len;
	memcpy (win->lines[yrel] + xrel, buff, l);
	return l;
}


int window_title_printf (Window *win, int xrel, char *format, ...)
{
	va_list ap;
	char buff[LINE_WIDTH + 1];
	int len, max, l;


	if (!win || xrel >= LINE_WIDTH)
		return 0;

	va_start (ap, format);
	vsnprintf (buff, sizeof (buff) - 1, format, ap);
	va_end (ap);
	
	len = strlen (buff);
	max = LINE_WIDTH - xrel;
	l = (len > max) ? max : len;
	memcpy (&win->title[xrel], buff, l);
	return l;
}


void window_set_title (Window *win, char *title)
{
	memset (win->title, ' ', LINE_WIDTH);
	win->title[LINE_WIDTH] = '\0';
	strncpy (win->title, title, strlen (title));

	window_set_title_attributes (win, DEF_COLOR, 0);
}


int window_cprintf (Window *win, int xrel, int yrel, int color, char *format, ...)
{
	va_list ap;
	char buff[LINE_WIDTH + 1];
	int len, max, l;


	if (!win || yrel >= win->nlines || xrel >= LINE_WIDTH)
		return 0;

	va_start (ap, format);
	vsnprintf (buff, sizeof (buff) - 1, format, ap);
	va_end (ap);
	
	len = strlen (buff);
	max = LINE_WIDTH - xrel;
	l = (len > max) ? max : len;
	memcpy (win->lines[yrel] + xrel, buff, l);
//	win->attributes[yrel][xrel] = attributes (color, 0) + 1;
//	win->attributes[yrel][xrel + len] = win->default_attributes + 1;
	window_line_set_attribs (win, yrel, xrel, xrel + len, color, 0);

	return l;
}


void window_title_clear (Window *win, int a, int b)
{
	memset (win->title + a, ' ', b - a);
}


void window_clear_line (Window *win, int line)
{
	memset (win->lines[line], ' ', LINE_WIDTH);
	win->lines[line][LINE_WIDTH] = '\0';
	window_line_set_attribs (win, line, 0, LINE_WIDTH, 0, 0);
}


void window_roll (Window *win, int lines, int direction)
{
	char *tmp;
	int *itmp;
	int i;


	switch (direction)
	{
		case DIR_DOWN:
			tmp = win->lines[0];
			itmp = win->attributes[0];
			for (i = 0; i < win->nlines - 1; i++)
			{
				win->lines[i] = win->lines[i + 1];
				win->attributes[i] = win->attributes[i + 1];
			}
	
			win->lines[win->nlines - 1] = tmp;
			win->attributes[win->nlines - 1] = itmp;
			break;
		
		case DIR_UP:
			tmp = win->lines[win->nlines - 1];
			itmp = win->attributes[win->nlines - 1];
			for (i = win->nlines - 1; i > 0; i--)
			{
				win->lines[i] = win->lines[i - 1];
				win->attributes[i] = win->attributes[i - 1];
			}
	
			win->lines[0] = tmp;
			win->attributes[0] = itmp;
			break;
	}
}


void screen_init (Screen *screen, int color)
{
	memset (screen, 0, sizeof (Screen));

	textmode_lib_init ();

	if (color)
		colors_init (color);
}


void screen_cleanup (Screen *screen)
{
	int i = 0;


	while (screen->wins[i])
		window_destroy (screen->wins[i++]);

	textmode_lib_cleanup ();
}


void screen_goto_selected (Screen *screen)
{
	int sel;


	sel = screen->wselected;
	cursor_goto (screen->wins[sel]->drawn_x, screen->wins[sel]->drawn_y);
}



void screen_refresh (Screen *screen)
{
	int i = 0, k;
	int line = 0;


	while (screen->wins[i])
	{
		k = window_draw_positioned (screen->wins[i], screen->wins[i]->x, line);
		screen->wins[i]->drawn_x = screen->wins[i]->x;
		screen->wins[i]->drawn_y = line;
		line += k;
		i++;
	}

	screen_goto_selected (screen);
	textmode_screen_refresh ();
}


void screen_clear (Screen *screen)
{
	textmode_screen_clear ();
}


void screen_add_window (Screen *screen, Window *win)
{
	screen->wins[screen->nwindows++] = win;
/*
	if (win->size_change_callback)
		win->size_change_callback (win, win->nlines, win->size_change_data);
*/
}


void window_enter (Window *win)
{
	if (win->enter_callback)
		win->enter_callback (win, win->enter_data);
}


void screen_select_window (Screen *screen, int dir)
{
	if (dir >= SELECT_N0)
	{
		screen->wselected = dir - SELECT_N0;
		if (screen->wselected >= screen->nwindows)
			screen->wselected = screen->nwindows - 1;
	}
	else switch (dir)
	{
		case SELECT_NEXT:
			screen->wselected++;
			if (screen->wselected >= screen->nwindows)
				screen->wselected = 0;
			break;
		
		case SELECT_PREV:
			screen->wselected--;
			if (screen->wselected < 0)
				screen->wselected = screen->nwindows - 1;
			break;
		
		case SELECT_FIRST:
			screen->wselected = 0;
			break;
		
		case SELECT_LAST:
			screen->wselected = screen->nwindows - 1;
			break;
	}

	screen_goto_selected (screen);
	window_enter (WIN_SELECTED (screen));
}


void window_add_scroll_callback (Window *win, void *callback, void *data)
{
	win->scroll_callback = callback;
	win->scroll_data = data;
}


void window_add_size_change_callback (Window *win, void *callback, void *data)
{
	win->size_change_callback = callback;
	win->size_change_data = data;
}


void window_scroll (Window *win, int lines, int direction)
{
	if (win->scroll_callback)
		win->scroll_callback (win, lines, direction, win->scroll_data);
}


void window_line_set_attribs (Window *win, int line, int x1, int x2, int color, int attr)
{
	int i;


	if (x2 > 79)
		x2 = 79;
	for (i = x1; i < x2; i++)
		win->attributes[line][i] = attributes (color, attr)+1;
	
	win->attributes[line][i] = win->default_attributes + 1;
}


// find how many lines are empty
// returns (MAX_LINES - lines_filled_by_windows)
int screen_count_empty_lines (Screen *screen)
{
	int i, c = 0;


	for (i = 0; i < screen->nwindows; i++)
	{
		if (screen->wins[i]->hidden)
			c++;
		else
			c += screen->wins[i]->nlines + 1;

		if (screen->wins[i]->no_title)
			c--;
	}
	
	return MAX_LINES - c;
}


int window_is_resizable (Window *win, int size_restriction)
{
	if (!win->hidden && !win->fixed_size &&
		  (!size_restriction || (size_restriction && win->nlines > 1)))
		return TRUE;
	else
		return FALSE;	
}



// find resizable and visible window
int screen_find_resizable (Screen *screen, int n, int size_restriction)
{
	int i;


	for (i = n + 1; i < screen->nwindows; i++)
		if (window_is_resizable (screen->wins[i], size_restriction))
				return i;
		
	for (i = n - 1; i >= 0; i--)
		if (window_is_resizable (screen->wins[i], size_restriction))
				return i;

	return -1;
}


void screen_expand (Screen *screen, int n)
{
	int k;


	k = screen_find_resizable (screen, n, FALSE);
	if (k >= 0)
		window_expand (screen->wins[k], screen_count_empty_lines (screen));
}


void screen_collapse (Screen *screen, int n)
{
	int k;


	do
	{
		k = screen_find_resizable (screen, n, TRUE);
		if (k < 0)
		{
			if (!window_is_resizable (screen->wins[n], TRUE))
				// bummer
				return;

			k = n;
		}
		window_collapse (screen->wins[k], -screen_count_empty_lines (screen));
	}
	while (screen_count_empty_lines (screen) != 0);
}


void screen_hide_window (Screen *screen, int sw, int n)
{
	if (n >= screen->nwindows || n < 0 || screen->wins[n]->always_visible)
		return;

	// only possible if there is at least one resizable window to fill the screen
	if (screen_find_resizable (screen, n, FALSE) < 0)
		return;

	switch (sw)
	{
		case VIS_HIDE:
			screen->wins[n]->hidden = TRUE;
			break;
		
		case VIS_SHOW:
			screen->wins[n]->hidden = FALSE;
			break;
		
		case VIS_SWITCH:
			screen->wins[n]->hidden = !screen->wins[n]->hidden;
			break;
	}

	if (screen->wins[n]->hidden)
		screen_expand (screen, n);
	else
		screen_collapse (screen, n);

	screen_clear (screen);
	screen_redraw (screen);
}


void screen_hide_window_selected (Screen *screen, int sw)
{
	screen_hide_window (screen, sw, screen->wselected);
}


void window_refresh (Window *win)
{
	if (!win->hidden && win->refresh_callback)
			win->refresh_callback (win, win->refresh_data);
}


void window_expand (Window *win, int n)
{
	if (win->fixed_size)
		return;

	if (n > MAX_LINES - win->nlines - 1)
		n = MAX_LINES - win->nlines - 1;

	win->nlines += n;
	if (win->size_change_callback)
		win->size_change_callback (win, win->nlines, win->size_change_data);
}


void window_collapse (Window *win, int n)
{
	if (win->fixed_size)
		return;

	if (n > win->nlines - 1)
		win->nlines = 1;
	else	
		win->nlines -= n;

	if (win->size_change_callback)
		win->size_change_callback (win, win->nlines, win->size_change_data);
}


void screen_expand_window (Screen *screen, int n)
{
	int l;


	if (WIN_SELECTED (screen)->fixed_size)
		return;

	l = screen_find_resizable (screen, screen->wselected, TRUE);
	if (l < 0)
		return;

	if (n > screen->wins[l]->nlines - 1)
		n = screen->wins[l]->nlines - 1;
			
	window_collapse (screen->wins[l], n);

	window_expand (WIN_SELECTED (screen), n);
	screen_refresh (screen);
}


void screen_collapse_window (Screen *screen, int n)
{
	int l;


	if (WIN_SELECTED (screen)->fixed_size)
		return;
	
	l = screen_find_resizable (screen, screen->wselected, FALSE);
	if (l < 0)
		return;

	if (n > WIN_SELECTED (screen)->nlines - 1)
		n = WIN_SELECTED (screen)->nlines - 1;
			
	window_expand (screen->wins[l], n);

	window_collapse (WIN_SELECTED (screen), n);
	screen_refresh (screen);
}


void screen_redraw (Screen *screen)
{
	int i;


	for (i = 0; i < screen->nwindows; i++)
		window_refresh (screen->wins[i]);
	
	screen_refresh (screen);
}


void screen_redraw_window (Screen *screen, Window *win)
{
	window_refresh (win);
	screen_refresh (screen);
}


void window_add_keypress_callback (Window *win, void *callback, void *data)
{
	win->keypress_callback = callback;
	win->keypress_data = data;
}


int window_handle_keypress (Window *win, int key, int modifiers)
{
	if (win->keypress_callback)
		return win->keypress_callback (win, key, modifiers, win->keypress_data);
	
	return FALSE;
}


void window_add_refresh_callback (Window *win, void *callback, void *data)
{
	win->refresh_callback = callback;
	win->refresh_data = data;
}


void screen_add_keypress_callback (Screen *screen, void *callback, void *data)
{
	screen->keypress_callback = callback;
	screen->keypress_data = data;
}


void screen_add_unhandled_keypress_callback (Screen *screen, void *callback, void *data)
{
	screen->unhandled_keypress_callback = callback;
	screen->unhandled_keypress_data = data;
}


int screen_handle_keypress (Screen *screen, int key, int modifiers)
{
	if (window_handle_keypress (WIN_SELECTED (screen), key, 0))
		return TRUE;

	if (screen->keypress_callback)
		return screen->keypress_callback (screen, key, modifiers, screen->keypress_data);

	return FALSE;
}


void screen_set_cursor_relative (Screen *screen, Window *win, int x, int y)
{
	cursor_goto (win->drawn_x + x, win->drawn_y + y);
}


void window_add_enter_callback (Window *win, void *callback, void *data)
{
	win->enter_callback = callback;
	win->enter_data = data;
}


// return TRUE if input was handled
int screen_handle_input (Screen *screen, int key)
{
	if (screen_handle_keypress (screen, key, 0))
		return TRUE;

	switch (key)
	{
		case KEY_DOWN:
		case KEY_UP:
			if (!WIN_SELECTED (screen)->hidden && WIN_SELECTED (screen)->nlines >= 1)
			{
				window_scroll (WIN_SELECTED (screen), 1, (key == KEY_DOWN) ? DIR_DOWN : DIR_UP);
				screen_refresh (screen);
				return TRUE;
			}
			return FALSE;

		case KEY_NPAGE:
		case KEY_PPAGE:
			if (!WIN_SELECTED (screen)->hidden && WIN_SELECTED (screen)->nlines >= 1)
			{
				if (WIN_SELECTED (screen)->nlines - 1 == 0)
					window_scroll (WIN_SELECTED (screen), WIN_SELECTED (screen)->nlines,
												 (key == KEY_NPAGE) ? DIR_DOWN : DIR_UP);
				else
					window_scroll (WIN_SELECTED (screen), WIN_SELECTED (screen)->nlines - 1,
												 (key == KEY_NPAGE) ? DIR_DOWN : DIR_UP);
				screen_refresh (screen);
				return TRUE;
			}
			return FALSE;

		case KEY_HOME:
		case KEY_END:
			if (!WIN_SELECTED (screen)->hidden && WIN_SELECTED (screen)->nlines >= 1)
			{
				window_scroll (WIN_SELECTED (screen), 0,
												(key == KEY_HOME) ? DIR_FIRST : DIR_LAST);
				screen_refresh (screen);
				return TRUE;
			}
			return FALSE;

		case KEY_BTAB:
			screen_select_window (screen, SELECT_PREV);
			return TRUE;

		// ordinary tab
		case 0x09:
			screen_select_window (screen, SELECT_NEXT);
			screen_refresh (screen);
			return TRUE;
		
		// tilde
		case '`':
			screen_hide_window_selected (screen, VIS_SWITCH);
			return TRUE;
		
		case '+':
		case '=':
			if (!WIN_SELECTED (screen)->hidden)
				screen_expand_window (screen, 1);
			return TRUE;
		
		case '-':
			if (!WIN_SELECTED (screen)->hidden)
				screen_collapse_window (screen, 1);
			return TRUE;
		
		case '1':
		case '2':
		case '3':
		case '4':
			screen_hide_window (screen, VIS_SWITCH, key - '1');
			return TRUE;
		
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			screen_select_window (screen, SELECT_N0 + key - '5');
			return TRUE;

		default:
			if (screen->unhandled_keypress_callback)
				return screen->unhandled_keypress_callback (screen, key, 0, screen->unhandled_keypress_data);
			else
				return FALSE;
	}
}
