#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <errno.h>
#include <locale.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define _GOLC_VERSION "1.0.3"

#define OUTFILE_BUF_SIZE 512

/// Label container for our return values
enum error_codes {
  E_SUCCESS,
  E_CURSES,
  E_MOUSE,
  E_INPUT,
  E_OPTION,
  E_IO,
};

struct parsed_args {
  bool help;
  bool version;
  bool wrapping;
  char outfile[OUTFILE_BUF_SIZE];
  char *infile;
  wchar_t active;
  wchar_t inactive;
};

//------------------ CLI ------------------

enum error_codes parse_args(int, char **, struct parsed_args *);

// TODO: These two
void show_help();

void show_version();

//------------------ Util ------------------

struct InfileData {
  wchar_t *scr;
  int lines;
  int cols;
};

enum error_codes read_scr_from_file(struct parsed_args *, struct InfileData *);

enum error_codes write_scr_to_file(struct parsed_args *, wchar_t *, int, int);

bool nstrcmp(char *, int, ...);

float diff_ms(struct timeval, struct timeval);

int min(int, int);

//------------------ Screen ------------------

enum error_codes init_screen();

wchar_t *resize(wchar_t *, struct parsed_args *, int, int);

void iterate(wchar_t *, struct parsed_args *);

int count_neighbours(wchar_t *, struct parsed_args *, int, int);

void draw_full_scr(const wchar_t *);

void draw_msg_buf(char *);

bool cell_is_active(wchar_t *, struct parsed_args *);

int flip_by_cell(wchar_t *, struct parsed_args *);

wchar_t *get_cell(wchar_t *, int, int);

int flip_by_cords(wchar_t *, struct parsed_args *, int, int);

void make_backup(wchar_t **, wchar_t *);

#endif // _SCREEN_H_
