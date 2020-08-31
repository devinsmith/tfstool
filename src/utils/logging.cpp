/*
 * Copyright (c) 2008 Devin Smith <devin@devinsmith.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <time.h>
#include <string.h>

#include "logging.h"

static int			__LogLvl;
static FILE	*	__LogFile;

static int __log_cleanup_has_been_added = 0;

void log_init(void)
{
  __LogLvl = -1;
  __LogFile = NULL;

  if (!__log_cleanup_has_been_added) {
    if (atexit(log_exit))
      return;
    __log_cleanup_has_been_added = 1;
  }
  __LogLvl = 0;

  return;
}

void log_set_level(int lvl)
{
	__LogLvl = lvl;
	return;
}

void log_exit(void)
{
  if (__LogFile != NULL)
  {
    fflush(__LogFile);
    fclose(__LogFile);
  }
}

int log_set_logfile(const char *file)
{
	__LogFile = fopen(file, "w");
	if (__LogFile == NULL)
		return -1;
	return 0;
}

void log_msg(int lvl, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  if (__LogLvl >= lvl) {
    if (__LogFile != NULL) {
      // fprintf it
      vfprintf(__LogFile, fmt, ap);
      fprintf(__LogFile, "\n");
      fflush(__LogFile);
    }
    else {
      /* Just output to stdout */
      (void) vfprintf(stdout, fmt, ap);
      fprintf(__LogFile, "\n");
    }
  }
  va_end(ap);
  return;
}

void log_msgraw(int lvl, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  if (__LogLvl >= lvl) {
    if (__LogFile != NULL) {
      // fprintf it
      vfprintf(__LogFile, fmt, ap);
      fflush(__LogFile);
    }
    else {
      /* Just output to stdout */
      (void) vfprintf(stdout, fmt, ap);
    }
  }
  va_end(ap);
  return;
}

void
log_tmsg(int lvl, const char *fmt, ...)
{
	va_list ap;
	time_t now;
	struct tm *tm_now;

	va_start(ap, fmt);
	if (__LogLvl >= lvl) {

		now = time(NULL);
		tm_now = localtime(&now);

		if (__LogFile != NULL) {
			fprintf(__LogFile, "%04d.%02d.%02d-%02d:%02d:%02d ",
			    tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
			    tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);

			// fprintf it
			vfprintf(__LogFile, fmt, ap);
      fprintf(__LogFile, "\n");
			fflush(__LogFile);
		}
		else {
			/* Just output to stdout */
			fprintf(stdout, "%04d.%02d.%02d-%02d:%02d:%02d ",
			    tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
			    tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
			(void) vfprintf(stdout, fmt, ap);
      fprintf(stdout, "\n");
		}
	}
	va_end(ap);
	return;
}

void log_flush(void)
{
  if (__LogFile != NULL) {
    fflush(__LogFile);
  } else {
    fflush(stdout);
  }
}

