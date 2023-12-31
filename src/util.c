/*
 *    util.c
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

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pmap_debug.h"
#include "util.h"

/* -------------------------------------------- */

/**
 * Trims leading whitespace characters from the beginning of a given string and
 * returns a pointer to the modified string.
 *
 * @param s  The input string to be trimmed.
 *
 * @return   A pointer to the input string with leading whitespace removed.
 *           The input string is modified in place, and the returned pointer
 * points to the modified string.
 */
char *pmap_ut_ltrim(char *s) {
  while (isspace(*s))
    s++;
  return s;
}

/* -------------------------------------------- */

/**
 * Trims trailing whitespace characters from the end of a given string in place.
 *
 * @param s  The input string to be trimmed.
 *
 * @return   A pointer to the input string with trailing whitespace removed.
 *           The input string is modified in place, and the returned pointer
 * points to the same string.
 */
char *pmap_ut_rtrim(char *s) {
  char *back = s + strlen(s);
  while (isspace(*--back))
    ;
  *(back + 1) = '\0';
  return s;
}

/* -------------------------------------------- */

/**
 * Trims leading and trailing whitespace characters from a given string and
 * returns a new pointer of the string.
 *
 * WARNING:
 * it operates on the same input string and returns a different pointer to the
 * same string with leading and trailing whitespace removed. It's important for
 * users to be aware that the pointer to the string is modified, and if the
 * input string is dynamically allocated, the original pointer may be
 * invalidated.
 *
 * @param s  The input string to be trimmed.
 *
 * @return   A pointer to the input string with leading and trailing whitespace
 * removed. The input string is modified in place, and the returned pointer
 * points to the same string. Care should be taken when passing dynamically
 * allocated strings, as the original pointer may become invalid.
 */
char *pmap_ut_trim(char *s) {

  /* Trim both side left and right */
  return pmap_ut_ltrim(pmap_ut_rtrim(s));
}

/* -------------------------------------------- */

/**
 * Extracts a substring from a given snippet of text between specified start and
 * end markers, trims leading whitespace, and stores the result in a buffer.
 *
 * @param startTxt     The starting marker to identify the beginning of the
 * desired substring.
 * @param endTxt       The ending marker to identify the end of the desired
 * substring.
 * @param snippetTxt   The input snippet of text in which to search for the
 * markers.
 * @param buffer       The character array where the extracted and trimmed
 * substring will be stored.
 * @param len          The maximum length of the buffer, including the
 * null-terminator.
 *
 * @return             0 if the operation was successful, indicating that a
 * valid substring was found and copied to the buffer. 1 if there was an error,
 * indicating that the start or end markers were not found or 2 if the resulting
 * substring was too long to fit in the buffer.
 */
int pmap_ut_substr(const char *startTxt, const char *endTxt,
                   const char *snippetTxt, char *buffer, int len) {

  const char *start = strstr(snippetTxt, startTxt);
  if (start == NULL) {
    return 1;
  }

  const char *end = strstr(start + 1, endTxt);

  if (start != NULL && end != NULL && start < end) {
    start += strlen(startTxt);
    int clen = end - start;
    if (clen < len) {
      char *trimmed = pmap_ut_ltrim((char *)start);
      clen = end - trimmed;
      strncpy(buffer, trimmed, clen);
      buffer[clen] = '\0';
    } else {
      return 2;
    }
  } else {
    return 1;
  }

  return 0;
}

/* -------------------------------------------- */

/**
 * Parse a URL and populate a pmap_url_comp_t structure with its components.
 *
 * URL example: http://192.168.1.1:53055/rootDesc.xml
 *
 * NOTE: The caller is responsible for freeing the memory allocated for
 * url component by calling 'pmap_ut_url_free' function.
 *
 * @param url     The input URL to be parsed.
 * @return        A pointer to a pmap_url_comp_t structure where the URL
 * components will be stored or NULL an error code (EINVALIDURL) is stored in
 * global errno variable.
 */
pmap_url_comp_t *pmap_ut_parse_url(const char *url) {

  /* Make a copy to avoid modifying the original string */
  char *copy = strdup(url);
  char *token, *rest;
  pmap_url_comp_t *ucomp = calloc(1, sizeof(pmap_url_comp_t));

  /* Set default port */
  ucomp->port = 80;

  // Parse the scheme
  token = strtok_r(copy, ":", &rest);
  if (token) {
    ucomp->scheme = token;
  } else {
    ucomp->scheme = NULL;
    free(copy);
    free(ucomp);
    errno = EINVALIDURL;
    return NULL;
  }

  // Parse the host and port
  if (strstr(rest, "//") == rest) {
    token = strtok_r(NULL, ":", &rest);
    if (rest) {
      ucomp->host = token + 2;
      token = strtok_r(NULL, "/", &rest);
      if (token) {
        ucomp->port = atoi(token);
      }
    } else {
      token = strtok_r(token, "/", &rest);
      if (rest) {
        ucomp->host = token;
      }
    }
  }

  // Parse the path
  ucomp->path = rest;

  return ucomp;
}

/**
 * Free the memory allocated for a pmap_url_comp_t structure.
 *
 * It frees the components of the structure, including the scheme, and the
 * structure itself. It is safe to call this function with a NULL pointer.
 *
 * @param url A pointer to the pmap_url_comp_t structure to be freed.
 */
void pmap_ut_free_url(pmap_url_comp_t *url) {

  if (url != NULL) {

    if (url->scheme) {
      free(url->scheme);
    }

    if (url->crtl_url) {
      free(url->crtl_url);
    }

    PMAP_DEBUG_LOG("Cleanup url component %s,%s\n", url->host, url->path);
    free(url);
  }
}

/**
 * Convert a 32-bit IP address to a human-readable string representation.
 *
 * This function takes a 32-bit IP address in network byte order (big-endian)
 * and converts it into a human-readable string representation in dotted-decimal
 * format (e.g., "192.168.1.1").
 *
 * @param ip The 32-bit IP address to be converted to a string.
 *
 * @return A pointer to a character array containing the human-readable IP
 * address string.
 *         - The returned pointer points to a static character array, so it
 * should not be modified or freed.
 *
 */
char *pmap_ut_inet_ntoa(uint32_t ip) {

  unsigned char bytes[4];
  static char b[18];

  bytes[0] = ip & 0xFF;
  bytes[1] = (ip >> 8) & 0xFF;
  bytes[2] = (ip >> 16) & 0xFF;
  bytes[3] = (ip >> 24) & 0xFF;

  snprintf(b, sizeof(b), "%u.%u.%u.%u", bytes[0], bytes[1], bytes[2], bytes[3]);

  return b;
}

#if PMAP_DEBUG_LOG_DEBUG
void pmap_ut_dump_hex(const void *data, size_t size) {
  char ascii[17];
  size_t i, j;
  ascii[16] = '\0';
  for (i = 0; i < size; ++i) {
    printf("%02X ", ((unsigned char *)data)[i]);
    if (((unsigned char *)data)[i] >= ' ' &&
        ((unsigned char *)data)[i] <= '~') {
      ascii[i % 16] = ((unsigned char *)data)[i];
    } else {
      ascii[i % 16] = '.';
    }
    if ((i + 1) % 8 == 0 || i + 1 == size) {
      printf(" ");
      if ((i + 1) % 16 == 0) {
        printf("|  %s \n", ascii);
      } else if (i + 1 == size) {
        ascii[(i + 1) % 16] = '\0';
        if ((i + 1) % 16 <= 8) {
          printf(" ");
        }
        for (j = (i + 1) % 16; j < 16; ++j) {
          printf("   ");
        }
        printf("|  %s \n", ascii);
      }
    }
  }
}
#endif
