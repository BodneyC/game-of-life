#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "golc.h"

/// Sleep in microseconds between loops
#define REFRESH_RATE_US 1000

/// Buf size max for bottom-line message
#define MSG_BUF_LEN 512

/// Main curses loop handling all IO
enum error_codes main_loop(struct parsed_args *args, wchar_t *scr) {
  enum error_codes ec = E_SUCCESS;

  int lines = LINES, cols = COLS;

  int scr_width = lines * cols;
  if (scr == NULL) {
    scr = malloc(scr_width * sizeof(wchar_t));
    wmemset(scr, args->inactive, scr_width);
  }

  wchar_t *scr_bak = NULL;

  refresh();

  bool running = false;
  bool begin = true;

  char msg_buf[MSG_BUF_LEN];

  struct timeval start, end;
  gettimeofday(&start, 0);

  long interval_ms = 200;

  draw_full_scr(scr);

  for (;;) {
    int c = wgetch(stdscr);

    /// 'q' is our 'quit' character breaking the for{}
    if (c == 'q') {
      break;
    }

    /// Write state to file indicated by `-i` or `-o`
    if (c == 'w') {
      ec = write_scr_to_file(args, scr, lines, cols);
      if (ec == E_SUCCESS) {
        continue;
      } else {
        fprintf(stderr, "Error writing state to file\n");
        break;
      }
    }

    switch (c) {
    case ERR:
      /// No key pressed, do nothing
      if (begin) {
        snprintf(msg_buf, MSG_BUF_LEN, "Awaiting input...");
        begin = false;
      }
      break;

    case 'k':
    case '?':
      /// Show "help" in message buffer ('k' for 'keys')
      snprintf(msg_buf, MSG_BUF_LEN,
               "[r] run, [b] backup, [R] restore, [i] iterate, [w] write, [s] "
               "speed, [S] slow");
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
          snprintf(msg_buf, MSG_BUF_LEN, "Running, backup creation failed");
        } else {
          snprintf(msg_buf, MSG_BUF_LEN, "Running, screen has been backed up");
        }
      } else {
        snprintf(msg_buf, MSG_BUF_LEN, "Stopped running");
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
          make_backup(&scr_bak, scr);
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
      /// Perform a single iteration when not running
      if (running) {
        snprintf(msg_buf, MSG_BUF_LEN, "Cannot iterate while running");
      } else {
        iterate(scr, args);
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
      ///  - `scr` must be resized while trying to maintain state
      ///  - `lines` and `cols` must be reassigned
      ///  - The whole screen must be redrawn
      if (running) {
        snprintf(msg_buf, MSG_BUF_LEN,
                 "Screen resize while running, results may vary...");
      }
      endwin();
      init_screen();
      wchar_t *new_scr = resize(scr, args, lines, cols);
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
        fprintf(stderr, "Error processing mouse event\n");
        ec = E_MOUSE;
        break;
      }
      if (e.bstate == BUTTON1_PRESSED) {
        if (running) {
          // A single active box immediately inactivates, so stopping the thing
          //  running makes sense
          running = false;
        }
        flip_by_cords(scr, args, e.y, e.x);
        move(e.y, e.x);
        addnwstr(get_cell(scr, e.y, e.x), 1);
        int neighbours = count_neighbours(scr, args, e.y, e.x);
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
        iterate(scr, args);
        draw_full_scr(scr);
        start = end;
      }
    }

    // Sleep for a short period to avoid CPU overuse
    usleep(REFRESH_RATE_US);
  }

  if (scr) {
    free(scr);
  }

  if (scr_bak) {
    free(scr_bak);
  }

  return ec;
}

/// Entry-point
///  - Initialize ncurses screen
///  - Process key presses and mouse events and reacts accordingly
///  - Release in-use memory and terminated screen
int main(int argc, char **argv) {

  struct parsed_args args;
  if (parse_args(argc, argv, &args) == E_OPTION) {
    fprintf(stderr, "Error when parsing arguments\n");
    return E_OPTION;
  }

  if (args.help) {
    show_help();
    return E_SUCCESS;
  }

  if (args.version) {
    show_version();
    return E_SUCCESS;
  }

  if (args.outfile[0] == 0) {
    if (args.infile == NULL) {
      char outfile[] = "/tmp/golc.XXXXXX";
      errno = 0;
      if (mkstemp(outfile) == -1) {
        if (errno != 0) {
          fprintf(stderr, "Unable to create filename for outfile (%d)\n",
                  errno);
        } else {
          fprintf(stderr, "Unknown error creating outfile\n");
        }
        return E_IO;
      }
      strcpy(args.outfile, outfile);
    } else {
      strcpy(args.outfile, args.infile);
    }
    printf("No outfile provided, using '%s'\n", args.outfile);
  } else {
    printf("Using provided outfile '%s'\n", args.outfile);
  }

  FILE *outf = fopen(args.outfile, "ab+");
  if (!outf) {
    fprintf(stderr, "Could not create/open outfile (%s)\n", args.outfile);
    return E_IO;
  }
  fclose(outf);

  struct InfileData infile_data = {.scr = NULL};
  if (args.infile) {
    if (read_scr_from_file(&args, &infile_data) == E_IO) {
      fprintf(stderr, "Failed to read infile (%s) to screen buffer\n",
              args.infile);
      return E_IO;
    };
  }

  if (init_screen() == E_CURSES) {
    fprintf(stderr, "Error when initializing screen\n");
    return E_CURSES;
  }

  wchar_t *scr = NULL;

  if (args.infile) {
    scr = resize(infile_data.scr, &args, infile_data.lines, infile_data.cols);
    if (infile_data.scr) {
      free(infile_data.scr);
    }
  }

  enum error_codes ec = main_loop(&args, scr);

  endwin();

  return ec;
}
