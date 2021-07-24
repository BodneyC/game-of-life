// Note: '=' not supported, i.e. -o=./myfile.txt

#include "golc.h"

#include <endian.h>
#include <stdarg.h>
#include <stdio.h>

void show_help() {
  fprintf(stderr,
          "golc - Conway's Game of Life in C\n"
          "\n"
          "    golc [-w] [-o <file>] [-i <file>] [--active A] [--inactive _]\n"
          "\n"
          "-h|--help)     Show this help message\n"
          "-v|--version)  Print version information\n"
          "-w|--wrapping) Should the grid wrap or end at the edge\n"
          "-o|--outfile)  File to the which the screen may be written\n"
          "-i|--infile)   File to the which the screen may be read\n"
          "--active|--active-char)      Active cell character (ascii)\n"
          "--inactive|--inactive-char)  Inactive cell character (ascii)\n");
}

void show_version() { fprintf(stderr, "%s\n", _GOLC_VERSION); }

void set_defaults(struct parsed_args *args) {
  args->help = false;
  args->version = false;
  args->wrapping = false;
  memset(args->outfile, 0, OUTFILE_BUF_SIZE);
  args->infile = NULL;
  args->active = u'▓';
  args->inactive = u' ';
  // For testing simple chars
  // args->active = u'A';
  // args->inactive = u'I';
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
        // Note:
        //
        // I've tried a load of things to get this working with wide chars...
        //  but no luck
        //
        // Attempt 1:
        //
        //     swprintf(&(args->active), sizeof(wchar_t), L"%lc", argv[i]);
        //
        // Attempt 2:
        //
        //     char *arg = calloc(sizeof(wchar_t), 1);
        //     size_t len = strlen(argv[i]) - 1;
        //     memcpy(arg + (sizeof(wchar_t) - len), argv[i], len);
        //     int i0 = (uint32_t)arg[0] << 24 | (uint32_t)arg[1] << 16 |
        //              (uint32_t)arg[2] << 8 | (uint32_t)arg[3] << 0;
        //     free(arg);
        //
        // Attempt 3:
        //
        //   The actual value that works is:
        //
        //       0x00002593
        //
        //   But dumping the hex of --active ▓ gives:
        //
        //       ffffffe2
        //       ffffff96
        //       ffffff93
        //
        //   I can see the 0x93... but where in god's name is the 0x25(00)
        //
        // Attempt _n_:
        //
        //   I also tried a load of things I can't remember now so I can't write
        //    then down here
        args->active = argv[i][0];

      } else if (nstrcmp(opt, 2, "--inactive", "--inactive-char")) {
        if (++i == argc) {
          fprintf(stderr, "[CLI] Option (%s) has no string", opt);
        }
        // See above...
        args->inactive = argv[i][0];

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
