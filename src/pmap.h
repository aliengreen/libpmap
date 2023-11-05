/*
 *    pmap.h
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

#ifndef PMAP_H
#define PMAP_H

#include <stdint.h>

#include "buffer.h"
#include "pmap_cfg.h"
#include "util.h"

/* Error codes */
#define EINVALIDURL 200 /* Invalid URL */

#define PMAP_UPNP_ACTION_ADDPORT 1
#define PMAP_UPNP_ACTION_DELPORT 2
#define PMAP_UPNP_ACTION_GETEXTIP 3

#define PMAP_UPNP_LIST_ALL 0
#define PMAP_UPNP_LIST_IGD 1

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

typedef struct pmap_t_ {

  char *buffer;
  int len;

  char *tmp_buffer;
  int tmp_len;

} pmap_t;

typedef struct pmap_field_t_ {
  int external_port;
  int internal_port;
  char protocol[4];
  uint32_t internal_ip;
  uint32_t gateway_ip;
  int lifetime_sec;
} pmap_field_t;

void pmap_http_set_debug(uint8_t debug);
int pmap_list_upnp(pmap_url_comp_t **urls, uint8_t only_igds);
int pmap_list_igd(pmap_url_comp_t **urls);
void pmap_list_free(pmap_url_comp_t *urls);

int pmap_req_ctrlurl(pmap_url_comp_t *ucmp, char *ctrl_url, int size);
int pmap_upnp_addport(pmap_field_t *pfield, char *error, int size);
int pmap_upnp_delport(pmap_field_t *pfield, char *error, int size);
int pmap_upnp_getexip(pmap_field_t *pfield, char *external_ip, int esize,
                      char *error, int size);
pbuffer_t *pmap_upnp_action(int action, pmap_field_t *pfield, int *http_status);

#endif // PMAP_H
