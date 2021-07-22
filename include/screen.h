#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <locale.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/// Label container for our return values
enum ErrorCodes {
  E_SUCCESS,
  E_CURSES,
  E_MOUSE,
  E_INPUT,
};

/// Wide char for "active" or "clicked" cells (set in `screen.c`)
extern wchar_t ACTIVE;

/// Wide char for "inactive" or "un-clicked" cells (set in `screen.c`)
extern wchar_t INACTIVE;

enum ErrorCodes init_screen();

wchar_t *resize(wchar_t *, int, int);

void iterate(wchar_t *);

int count_neighbours(wchar_t *, int, int);

void draw_full_scr(const wchar_t *);

void draw_msg_buf(char *);

bool cell_is_active(wchar_t *);

int flip_by_cell(wchar_t *);

wchar_t *get_cell(wchar_t *, int, int);

int flip_by_cords(wchar_t *, int, int);

#endif // _SCREEN_H_
