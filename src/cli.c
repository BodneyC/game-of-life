// Note: '=' not supported, i.e. -o=./myfile.txt

#include "golc.h"

#include <stdarg.h>
#include <stdio.h>

void show_help() { fprintf(stderr, "Help\n"); }

void show_version() { fprintf(stderr, "Version\n"); }

void set_defaults(struct parsed_args *args) {
  args->help = false;
  args->version = false;
  args->wrapping = false;
  memset(args->outfile, 0, OUTFILE_BUF_SIZE);
  args->infile = NULL;
  args->active = u'▓';
  args->inactive = u' ';
  // For testing simple chars
  /* args->active = u'A'; */
  /* args->inactive = u'I'; */
}

enum error_codes parse_args(int argc, char **argv, struct parsed_args *args) {
  set_defaults(args);

  enum error_codes ec = E_SUCCESS;

  for (int i = 1; i < argc; i++) {
    char *opt = argv[i];
    if (opt[0] == '-') {
      if (nstrcmp(opt, 2, "-h", "--help")) {
        args->help = true;
        break;

      } else if (nstrcmp(opt, 2, "-v", "--version")) {
        args->version = true;
        break;

      } else if (nstrcmp(opt, 2, "-w", "--wrapping")) {
        args->wrapping = true;

      } else if (nstrcmp(opt, 2, "-o", "--outfile")) {
        if (++i == argc) {
          fprintf(stderr, "[CLI] Option (%s) has no string", opt);
        }
        strncpy(args->outfile, argv[i], OUTFILE_BUF_SIZE);

      } else if (nstrcmp(opt, 2, "-i", "--infile")) {
        if (++i == argc) {
          fprintf(stderr, "[CLI] Option (%s) has no string", opt);
        }
        args->infile = argv[i];

      } else if (nstrcmp(opt, 2, "--active", "--active-char")) {
        if (++i == argc) {
          fprintf(stderr, "[CLI] Option (%s) has no string", opt);
        }
        swprintf(&(args->active), sizeof(wchar_t), L"%lc", argv[i]);

      } else if (nstrcmp(opt, 2, "--inactive", "--inactive-char")) {
        if (++i == argc) {
          fprintf(stderr, "[CLI] Option (%s) has no string", opt);
        }
        swprintf(&(args->inactive), sizeof(wchar_t), L"%lc", argv[i]);

      } else {
        fprintf(stderr, "Unknown option (%s)", opt);
        ec = E_OPTION;
      }
    } else {
      fprintf(stderr, "Not an option (%s)", opt);
      ec = E_OPTION;
    }
  }

  return ec;
}
