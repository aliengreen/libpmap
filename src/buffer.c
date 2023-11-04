/*
 *    buffer.c
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

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "pmap_debug.h"

/**
 * Create a new pbuffer_t object with the specified length and initialize its
 * fields.
 *
 * This function allocates memory for a pbuffer_t structure and its buffer. If
 * memory allocation fails, it sets the errno to ENOMEM and returns NULL.
 *
 * @param size The length of the buffer to be created.
 *
 * @return A pointer to a newly created pbuffer_t object, or NULL if memory
 * allocation fails.
 */
pbuffer_t *pbfr_create(int size) {

  /* Allocate handler */
  pbuffer_t *pbfr = calloc(1, sizeof(pbuffer_t));
  if (NULL == pbfr) {
    errno = ENOMEM;
    PMAP_DEBUG_ERROR("Out of memory");
    pbfr_destroy(pbfr);
    return NULL;
  }

  /* Allocate buffer */
  pbfr->size = size;
  pbfr->offset = 0;
  pbfr->buffer = calloc(size, sizeof(uint8_t));
  if (NULL == pbfr->buffer) {
    errno = ENOMEM;
    PMAP_DEBUG_ERROR("Out of memory");
    pbfr_destroy(pbfr);
    return NULL;
  }

  return pbfr;
}

/**
 * Add formatted data to a pbuffer_t object.
 *
 * This function takes a pbuffer_t object and appends formatted data to its
 * buffer. It uses a variable argument list (va_list) and the provided format
 * string to format the data. The formatted data is added to the buffer starting
 * from the current offset within the buffer. The function returns the number of
 * characters written to the buffer.
 *
 * @param pbfr A pointer to a pbuffer_t object to which data will be added.
 * @param format A format string, similar to printf, specifying the format of
 * the data.
 * @param ... Additional arguments as required by the format string.
 * @return The number of characters written to the buffer, or -1 on error.
 */
int pbfr_add(pbuffer_t *pbfr, const char *format, ...) {

  va_list list;
  va_start(list, format);
  int len = vsnprintf(pbfr->buffer + pbfr->offset, pbfr->size - pbfr->offset,
                      format, list);
  pbfr->offset += len;
  va_end(list);

  return len;
}

/**
 * Append the contents of one pbuffer_t object to another.
 *
 * This function appends the contents of one pbuffer_t object (`pbfr_src`) to
 * another
 * (`pbfr`) by copying the data from the source buffer to the destination
 * buffer. The `offset` of the destination buffer is updated accordingly.
 *
 * @param pbfr A pointer to the destination pbuffer_t where the data will be
 * appended.
 * @param pbfr_src A pointer to the source pbuffer_t containing the data to be
 * appended.
 * @return The number of bytes appended to the destination buffer from the
 * source buffer.
 */
int pbfr_append(pbuffer_t *pbfr, pbuffer_t *pbfr_src) {

  if (pbfr != NULL && pbfr_src != NULL) {
    memmove(pbfr->buffer + pbfr->offset, pbfr_src->buffer, pbfr_src->offset);
    pbfr->offset += pbfr_src->offset;
  }

  return pbfr_src->offset;
}

/**
 * Destroy a pbuffer_t object and release its associated memory.
 *
 * This function is responsible for releasing the memory allocated for a
 * pbuffer_t object and its associated buffer. It first checks if the input
 * pointer is not NULL, and if so, it frees the buffer and then the pbuffer_t
 * object itself. After calling this function, the pbuffer_t object is no longer
 * valid, and its memory has been deallocated.
 *
 * @param pbfr A pointer to the pbuffer_t object to be destroyed.
 */
void pbfr_destroy(pbuffer_t *pbfr) {

  if (NULL != pbfr) {

    if (NULL != pbfr->buffer) {
      free(pbfr->buffer);
      pbfr->buffer = NULL;
    }

    free(pbfr);
  }
}
