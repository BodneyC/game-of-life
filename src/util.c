#include "golc.h"

/// Write the current screen state to the file indicated by command line
///  arguments
enum error_codes write_scr_to_file(struct parsed_args *args, wchar_t *scr,
                                   int lines, int cols) {
  enum error_codes ec = E_SUCCESS;

  errno = 0;
  FILE *fp = fopen(args->outfile, "wb");
  if (!fp) {
    fprintf(stderr, "Failed to open '%s' (%d)\n", args->outfile, errno);
    return E_IO;
  }

  // Using a char buffer as keeping it readable was a pain in the backside with
  //  `wchar_t`
  int width = (lines * cols) + (lines - 1);
  char *c_buf = malloc(width);

  for (int i = 0; i < lines; i++) {
    char *begin = c_buf + (i * cols) + i;
    for (int j = 0; j < cols; j++) {
      bool is_active = cell_is_active(scr + (i * cols) + j, args);
      *(begin + j) = is_active ? 'A' : '_';
    }
    if (i + 1 < lines) {
      *(begin + cols) = '\n';
    }
  }

  // Write the whole buffer to the file as binary
  errno = 0;
  int ferr = 0, n_read = 0;
  if ((n_read = fwrite(c_buf, 1, width, fp)) != width) {
    if (errno) {
      fprintf(stderr, "Character encoding error (%d)\n", errno);
    } else if ((ferr = ferror(fp))) {
      fprintf(stderr, "File write error (%d)\n", ferr);
    } else {
      fprintf(stderr, "Unknown read error (%d/%d)\n", n_read, width);
    }
    ec = E_IO;
  }

  if (c_buf) {
    free(c_buf);
  }

  fclose(fp);

  return ec;
}

/// Read a screen buffer from the given filename
///
/// If the `-i` option is given, the file is read and a screen buffer of the
///  size indicated by the contents is created
///
/// WARN: This function allocated memory (for the buffer), but does not free it,
///       that is left to the calling function
enum error_codes read_scr_from_file(struct parsed_args *args,
                                    struct InfileData *infile_data) {
  FILE *fp = fopen(args->infile, "rb");
  if (!fp) {
    fprintf(stderr, "Could not create/open file (%s)\n", args->infile);
    return E_IO;
  }
  fseek(fp, 0, SEEK_END);
  int fsize = ftell(fp);
  rewind(fp);

  int lines = 0, cols = 0;

  char ch;
  errno = 0;
  // Naive dimension calculation based on newlines
  while (!feof(fp)) {
    ch = fgetc(fp);
    if (errno) {
      fprintf(stderr, "Error reading stream (%d)\n", errno);
      fclose(fp);
      return E_IO;
    }
    if (ch == '\n') {
      lines++;
    } else if (!lines) {
      cols++;
    }
  }
  rewind(fp);

  // Buf size *should* be the file size minus newlines
  int buf_size = fsize - (lines - 1);
  if (cols == 0 || buf_size <= 0) {
    fprintf(stderr, "Invalid input buffer size %d (%d - %d)\n", buf_size, fsize,
            lines);
    fclose(fp);
    return E_IO;
  }

  // Just for sanity, no other purpose
  if (lines * cols != buf_size) {
    fprintf(stderr, "Calculated buf size (%d) does not match (%d), fsize: %d\n",
            buf_size, lines * cols, fsize);
  }

  printf("Input file of dimensions [%d, %d, (%d)]\n", lines, cols, buf_size);

  wchar_t *scr = malloc(buf_size * sizeof(wchar_t));

  // Translate simple chars ['_', 'A'] to args->{active,inactive}
  for (int i = 0; i < lines; i++) {
    for (int j = 0; j < cols; j++) {
      ch = fgetc(fp);
      if (ch == '\n') {
        fprintf(stderr, "Expected char but found '\\n' (%d, %d)\n", i, j);
        fclose(fp);
        return E_IO;
      }
      *(scr + (i * cols) + j) = ch == 'A' ? args->active : args->inactive;
    }
    if ((ch = fgetc(fp)) != '\n') {
      fprintf(stderr, "Expected newline but found '%c'\n", ch);
      fclose(fp);
      return E_IO;
    }
  }

  infile_data->scr = scr;
  infile_data->lines = lines;
  infile_data->cols = cols;

  fclose(fp);

  return E_SUCCESS;
}

int min(int a, int b) {
  if (a < b) {
    return a;
  }
  return b;
}

bool nstrcmp(char *opt, int nargs, ...) {

  va_list ap;
  va_start(ap, nargs);

  bool success = false;
  for (int i = 0; i < nargs; i++) {
    const char *str = va_arg(ap, const char *);
    if (strcmp(opt, str) == 0) {
      success = true;
      break;
    }
  }

  va_end(ap);

  return success;
}

/// Calculate the diff in microseconds between two `struct timevals`
float diff_ms(struct timeval t0, struct timeval t1) {
  return (t1.tv_sec - t0.tv_sec) * 1000.0f +
         (t1.tv_usec - t0.tv_usec) / 1000.0f;
}
