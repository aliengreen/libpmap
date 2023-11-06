/*
 *    pmap_cfg.h
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

#ifndef _PMAP_CFG_H
#define _PMAP_CFG_H

#include <stdint.h>

/* Config log */
#define PMAP_DEBUG_LOG_DEBUG 0
#define PMAP_DEBUG_LOG_ERROR 0
#define PMAP_PRINTF printf
/* ----------- */

/* Wait timeout in seconds */
#define PMAP_DEFAULT_WAIT_TIMEOUT 4

/* Error codes */
#define EINVALIDURL 200  /* Invalid URL */
#define EINVALIDPROT 201 /* Invalid Protocol checking for (UDP, TCP) */
/* NAT-PMP codes */
#define NPMP_OK 210                  /* Success */
#define ENPMP_UNSUPPORTED_VER 211    /* Unsupported Version */
#define ENPMP_NOT_AUTHORIZED 212     /* Not Authorized/Refused */
#define ENPMP_NETWORK_FAIL 213       /* Network Failure */
#define ENPMP_OUTOF_RESOURCE 214     /* Out of resources */
#define ENPMP_UNSUPPORTED_OPCODE 215 /* Unsupported opcode */

typedef struct pmap_field_t_ {
  int external_port;
  int internal_port;
  char protocol[4];
  uint32_t internal_ip;
  uint32_t gateway_ip;
  int lifetime_sec;
} pmap_field_t;

#endif
