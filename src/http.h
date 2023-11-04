/*
 *    http.h
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

#ifndef _HTTP_H
#define _HTTP_H

#include "buffer.h"
#include "pmap_cfg.h"

pbuffer_t *pmap_http_create(const char *method, const char *hostname, int port,
                            char *path);
int pmap_http_connect(const char *hostname, int port);
pbuffer_t *pmap_http_req(const char *hostname, int port, pbuffer_t *pbfr,
                         int *http_status);

#endif // _HTTP_H
