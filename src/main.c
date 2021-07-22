#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "screen.h"

/// Sleep in microseconds between loops
#define REFRESH_RATE_US 1000

/// Buf size max for bottom-line message
#define MSG_BUF_LEN 512

/// Calculate the diff in microseconds between two `struct timevals`
float diff_ms(struct timeval t0, struct timeval t1) {
  return (t1.tv_sec - t0.tv_sec) * 1000.0f +
         (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

/// Make a backup of scr into scr_bak
void make_backup(wchar_t **scr_bak, wchar_t *scr) {
  if (*scr_bak) {
    free(*scr_bak);
  }
  *scr_bak = wcsdup(scr);
}

/// Entry-point
///  - Initialize ncurses screen
///  - Process key presses and mouse events and reacts accordingly
///  - Release in-use memory and terminated screen
int main(void) {
  if (init_screen() == E_CURSES) {
    printf("Error when initializing screen");
    return E_CURSES;
  }

  int lines = LINES, cols = COLS;

  int scr_width = lines * cols;
  wchar_t *scr = malloc(scr_width * sizeof(wchar_t));
  wmemset(scr, INACTIVE, scr_width);

  wchar_t *scr_bak = NULL;

  refresh();

  bool running = false;
  bool begin = false;

  char msg_buf[MSG_BUF_LEN];

  enum ErrorCodes ec = E_SUCCESS;

  struct timeval start, end;
  gettimeofday(&start, 0);

  long interval_ms = 200;

  for (;;) {
    int c = wgetch(stdscr);

    /// 'q' is our 'quit' character breaking the for{}
    if (c == 'q') {
      break;
    }

    switch (c) {
    case ERR:
      /// No key pressed, do nothing
      if (!begin) {
        snprintf(msg_buf, MSG_BUF_LEN, "Awaiting input...");
        begin = true;
      }
      break;

    case 'k':
    case '?':
      /// Show "help" in message buffer ('k' for 'keys')
      snprintf(
          msg_buf, MSG_BUF_LEN,
          "[r] run, [b] backup, [R] restore, [i] iterate, [s] speed, [S] slow");
      break;

    case 'b':
      /// Make backup of current screen which can be restored using 'R'
      make_backup(&scr_bak, scr);
      if (scr_bak == NULL) {
        snprintf(msg_buf, MSG_BUF_LEN, "Backup creation failed");
      } else {
        snprintf(msg_buf, MSG_BUF_LEN, "Screen has been backed up");
      }
      break;

    case 'r':
      /// Toggle running state
      ///   If the system is not running, a backup is made
      if (!running) {
        make_backup(&scr_bak, scr);
        if (scr_bak == NULL) {
          snprintf(msg_buf, MSG_BUF_LEN, "Backup creation failed");
        } else {
          snprintf(msg_buf, MSG_BUF_LEN, "Screen has been backed up");
        }
      }
      running = !running;
      break;

    case 'R':
      /// Reset state to a previously made backup
      if (running) {
        running = false;
      }
      // Check that the screen hasn't been resized
      if (scr_bak) {
        if (lines * cols == LINES * COLS) {
          if (scr) {
            free(scr);
          }
          scr = scr_bak;
          scr_bak = NULL;
          draw_full_scr(scr);
          snprintf(msg_buf, MSG_BUF_LEN, "Screen size unchanged, state reset");
        } else {
          free(scr_bak);
          snprintf(msg_buf, MSG_BUF_LEN,
                   "Screen has been resized since last state save");
        }
      } else {
        snprintf(msg_buf, MSG_BUF_LEN, "No state to restore to");
      }
      break;

    case 'i':
      // Perform a single iteration when not running
      if (running) {
        snprintf(msg_buf, MSG_BUF_LEN, "Cannot iterate while running");
      } else {
        iterate(scr);
        draw_full_scr(scr);
      }
      break;

    case 's':
      /// Decrease interval between iterations effectively speeding up
      ///  iterations
      if (interval_ms - 10 > 0) {
        interval_ms -= 10;
        snprintf(msg_buf, MSG_BUF_LEN, "Interval reduced to %ld", interval_ms);
      } else {
        snprintf(msg_buf, MSG_BUF_LEN, "Interval cannot be reduced further");
      }
      break;

    case 'S':
      /// Increase interval between iterations effectively slowing iterations
      interval_ms += 10;
      snprintf(msg_buf, MSG_BUF_LEN, "Interval increased to %ld", interval_ms);
      break;

    case KEY_RESIZE: {
      /// The terminal has been resized
      ///  - scr must be resized while trying to maintain state
      ///  - lines and cols must be reassigned
      ///  - The whole screen must be drawn
      if (running) {
        snprintf(msg_buf, MSG_BUF_LEN,
                 "Screen resize while running, results may vary...");
      }
      wchar_t *new_scr = resize(scr, lines, cols);
      lines = LINES, cols = COLS;
      scr_width = lines * cols;
      if (scr) {
        free(scr);
      }
      scr = new_scr;
      draw_full_scr(scr);
      break;
    }

    case KEY_MOUSE: {
      /// A mouse event has taken place
      ///  - If the release of the button one, then toggle the active state of
      ///    that cell
      ///  - Redraw that one cell, no need to redraw the whole board
      MEVENT e;
      if (getmouse(&e) == ERR) {
        printf("Error processing mouse event");
        ec = E_MOUSE;
        break;
      }
      if (e.bstate == BUTTON1_RELEASED) {
        flip_by_cords(scr, e.y, e.x);
        move(e.y, e.x);
        addnwstr(get_cell(scr, e.y, e.x), 1);
        int neighbours = count_neighbours(scr, e.y, e.x);
        snprintf(msg_buf, MSG_BUF_LEN,
                 "Cell [%d, %d, (0x%04x)] with %d neighbour%c", e.y, e.x,
                 e.bstate, neighbours, neighbours == 1 ? ' ' : 's');
      }
      break;
    }

    default:
      /// Unknown key has been pressed, not really an issue for us
      snprintf(msg_buf, MSG_BUF_LEN, "Unknown key '%s' (%d)", keyname(c), c);
    }

    // Redraw the message buffer on *every* iteration
    draw_msg_buf(msg_buf);

    // To avoid input lag, calculate the difference between last iteration and
    //  now, if we're above the given threshold, perform an iteration
    if (running) {
      gettimeofday(&end, 0);
      if (diff_ms(start, end) > interval_ms) {
        iterate(scr);
        draw_full_scr(scr);
        start = end;
      }
    }

    // Sleep for a short period to avoid CPU overuse
    usleep(REFRESH_RATE_US);
  }

  endwin();

  if (scr) {
    free(scr);
  }

  if (scr_bak) {
    free(scr_bak);
  }

  return ec;
}
