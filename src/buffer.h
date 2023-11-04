/*
 *    buffer.h
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

#ifndef _BUFFER_H
#define _BUFFER_H

#define PBUFFER_DEFLEN     4096

typedef struct pbuffer_t_ {

  char *buffer;
  int size;
  int offset;

} pbuffer_t;

pbuffer_t *pbfr_create(int size);
int pbfr_add(pbuffer_t *pbfr, const char *format, ...);
int pbfr_append(pbuffer_t *pbfr, pbuffer_t *pbfr_src);
void pbfr_destroy(pbuffer_t *pbfr);

#endif // _BUFFER_H
