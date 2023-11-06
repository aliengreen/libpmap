/*
 *    pmap_npmp.h
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

#ifndef _PMAP_NPMP_H
#define _PMAP_NPMP_H

#include <stdint.h>

#include "buffer.h"
#include "pmap_cfg.h"
#include "util.h"

#define NAT_PMP_VERSION 0
#define NAT_PMP_SERVER_PORT 5351

/**
 * The use of the __attribute__((__packed__)) directive ensures that the data
 * is tightly packed without any padding.
 */

/**
 * Represents the common header for network management and port mapping packets.
 * Contains version information (8 bits) and an operation code (8 bits) to
 * specify the packet's purpose.
 */
typedef struct nmpm_pkt_header_ {
  uint8_t version;
  uint8_t op_code;
} __attribute__((__packed__)) nmpm_pkt_header;

/**
 * Represents a request packet for network management and port mapping.
 * Includes the common nmpm_pkt_header and additional fields for reserved data
 * (16 bits), internal port (16 bits), external port (16 bits), and port mapping
 * lifetime in seconds (32 bits).
 */
typedef struct nmpm_pkt_req_ {
  nmpm_pkt_header header;
  uint16_t reserverd;
  uint16_t internal_port;
  uint16_t external_port;
  uint32_t lifetime_sec;
} __attribute__((__packed__)) nmpm_pkt_req;

/**
 * Represents a response packet for obtaining external IP information.
 * Shares the common nmpm_pkt_header and includes fields for response code (16
 * bits), the time when the response started (32 bits), and the external IP
 * address (32 bits).
 */
typedef struct nmpm_pkt_exip_ {
  nmpm_pkt_header header;
  uint16_t res_code;
  uint32_t secs_start;
  uint32_t external_ip;
} __attribute__((__packed__)) nmpm_pkt_exip;

/**
 * Represents a response packet for network management and port mapping, often
 * in response to a port mapping request. Includes the common nmpm_pkt_header
 * and fields for response code (16 bits), the time when the response started
 * (32 bits), internal port (16 bits), external port (16 bits), and port mapping
 * lifetime in seconds (32 bits).
 */
typedef struct nmpm_pkt_resp_ {
  nmpm_pkt_header header;
  uint16_t res_code;
  uint32_t secs_start;
  uint16_t internal_port;
  uint16_t external_port;
  uint32_t lifetime_sec;
} __attribute__((__packed__)) nmpm_pkt_resp;

int pmap_npmp_getexip(pmap_field_t *pfield, char *external_ip, int esize,
                      char *error, int size);
int pmap_npmp_addport(pmap_field_t *pfield, char *error, int size);
int pmap_npmp_delport(pmap_field_t *pfield, char *error, int size);

#endif // _PMAP_NPMP_H
