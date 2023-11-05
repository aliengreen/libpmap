/*
 *    util.h
 *
 *    Copyright (c) 2023 Alien Green LLC
 *
 *    This file is part of Mostat.
 *
 *    Mostat is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Mostat is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Mostat. If not, see <http://www.gnu.org/licenses/>.
 *
 *    ASCII font see http://patorjk.com/software/taag/#p=display&f=3D-ASCII
 */

#ifndef _UTIL_H
#define _UTIL_H

#include "pmap_errno.h"
#include <stdint.h>

typedef struct pmap_url_comp_t_ {

  struct pmap_url_comp_t_ *next;

  char *scheme;
  char *host;
  int port;
  char *path;
  char *crtl_url;
} pmap_url_comp_t;

#define PMAP_COMPARE_URL_COMP(a, b)                                            \
  (strcmp(a->host, b->host) == 0 && strcmp(a->path, b->path) == 0 &&           \
   a->port == b->port)

char *pmap_ut_ltrim(char *s);
char *pmap_ut_rtrim(char *s);
char *pmap_ut_trim(char *s);
int pmap_ut_substr(const char *startTxt, const char *endTxt,
                   const char *xmlSnippet, char *buffer, int len);
int pmap_ut_parse_url(const char *url, pmap_url_comp_t *ucomp);
void pmap_ut_free_url(pmap_url_comp_t *url);
char *pmap_ut_inet_ntoa(uint32_t ip);
#endif // _UTIL_H
