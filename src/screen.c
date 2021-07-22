#include "screen.h"

wchar_t ACTIVE = u'▓';
wchar_t INACTIVE = u' ';

/// Initialize the ncurses screen, enables:
///  - Wide-character input
///  - Non-blocking input (`getch`, screen resize, etc.)
///  - Mouse input, specifically for button one (left-click)
enum ErrorCodes init_screen() {
  setlocale(LC_ALL, "");

  initscr();
  cbreak();
  noecho();
  curs_set(0);

  if (nodelay(stdscr, TRUE) == ERR) {
    printf("Could not set non-blocking input for stdscr\n");
    return E_CURSES;
  }

  if (keypad(stdscr, TRUE) == ERR) {
    printf("Could not set keypad for stdscr\n");
    return E_CURSES;
  }

  mmask_t mmask = mousemask(BUTTON1_CLICKED, NULL);
  if (!mmask) {
    printf("Could not set button one input\n");
    return E_CURSES;
  }

  clear();

  return E_SUCCESS;
}

int min(int a, int b) {
  if (a < b) {
    return a;
  }
  return b;
}

/// Upon a terminal resize event, resize the screen buffer. This should be a
///  simple enough process, but keeping the state of the original buffer within
///  the new buffer added a little complexity
///
/// Here, we are copying `min(old_cols, new_cols)` from the line offset of
/// `old_screen` into the line in `new_screen`
wchar_t *resize(wchar_t *_scr, int _lines, int _cols) {
  endwin();
  init_screen();
  int scr_width = LINES * COLS;
  wchar_t *scr = malloc(scr_width * sizeof(wchar_t));
  wmemset(scr, INACTIVE, scr_width);
  int min_cols = min(_cols, COLS);
  for (int i = 0; i < LINES - 1; i++) {
    if ((i + 1) * _cols > (_lines * _cols) - 1 ||
        (i + 1) * COLS > scr_width - 1) {
      printf("[resize] Out of space in new arr (%d/%d)\n", i, _lines);
      break;
    }
    wcsncpy(scr + (i * COLS), _scr + (i * _cols), min_cols);
  }
  return scr;
}

/// Iterate the screen buffer once by the game of life rules
///  - Active cells with 1,4..8 neighbours become inactive
///  - Inactive cells with 3 neighbours become active
void iterate(wchar_t *scr) {
  int scr_width = (LINES - 1) * COLS;
  // We don't want modifications to the original buffer until we know how the
  //  whole buffer must change, ergo store the values which must be flipped in a
  //  new buffer
  wchar_t **flip_arr = calloc(scr_width, sizeof(wchar_t));
  int flip_idx = 0;
  for (int y = 0; y < LINES - 1; y++) {
    for (int x = 0; x < COLS; x++) {
      int neighbours = count_neighbours(scr, y, x);
      wchar_t *cell = get_cell(scr, y, x);
      bool active = cell_is_active(cell);
      if (active) {
        if (neighbours < 2 || neighbours > 3) {
          flip_arr[flip_idx] = cell;
          flip_idx++;
        }
      } else if (neighbours == 3) {
        flip_arr[flip_idx] = cell;
        flip_idx++;
      }
    }
  }
  // Iterate the flip buffer mentioned above and flip the cells referred to by
  //  each pointer
  for (int i = 0; i < scr_width; i++) {
    if (flip_arr[i] == NULL) {
      break;
    }
    flip_by_cell(flip_arr[i]);
  }
  if (flip_arr) {
    free(flip_arr);
  }
}

/// Calculate the count of active neighbours surrounding a particular cell
///  - Despite the game of life existing on an infinite grid, this one wraps...
int count_neighbours(wchar_t *scr, int y, int x) {
  int neighbours = 0;
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      // We don't count an active cell as a neighbour of itself
      if (i == 0 && j == 0) {
        continue;
      }
      // Calculate true x and y coords, wrapping as described above
      int _y = (y + i) % (LINES - 1);
      if (_y < 0) {
        _y = (LINES - 1) + _y;
      }
      int _x = (x + j) % COLS;
      if (_x < 0) {
        _x = COLS + _x;
      }
      // printf("(%d, %d) ", _y, _x);
      neighbours += cell_is_active(get_cell(scr, _y, _x));
    }
  }
  // printf("\n");
  return neighbours;
}

/// Draw the entire screen buffer to the terminal, line by line
void draw_full_scr(const wchar_t *scr) {
  wchar_t line_buf[COLS];
  for (int i = 0; i < LINES - 1; i++) {
    wcsncpy(line_buf, scr + (i * COLS), COLS);
    move(i, 0);
    addnwstr(line_buf, COLS);
  }
}

/// Draw the message buffer to the last line of the view
void draw_msg_buf(char *msg_buf) {
  move(LINES - 1, 0);
  insertln();
  addstr(msg_buf);
  clrtoeol();
}

bool cell_is_active(wchar_t *cell) { return *cell == ACTIVE; }

int flip_by_cell(wchar_t *cell) {
  if (cell_is_active(cell)) {
    *cell = INACTIVE;
  } else {
    *cell = ACTIVE;
  }
  return *cell;
}

wchar_t *get_cell(wchar_t *scr, int y, int x) { return scr + ((y * COLS) + x); }

int flip_by_cords(wchar_t *scr, int y, int x) {
  return flip_by_cell(get_cell(scr, y, x));
}
