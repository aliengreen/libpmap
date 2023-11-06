/*
 *    pmap_npmp.c
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

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "buffer.h"
#include "http.h"
#include "pmap_debug.h"
#include "pmap_npmp.h"
#include "upnp_msg.h"
#include "util.h"

static const char *npmp_res_codes[] = {
    "Success",         "Unsupported Version", "Not Authorized/Refused",
    "Network Failure", "Out of resources",    "Unsupported opcode",
};

static const char npmp_fatal_err[] = "Fatal Error";

/* -------------------------------------------- */

int _pmap_npm_setup_socket(struct sockaddr_in *npmp, uint32_t gateway_ip) {

  int sockfd = 0;
  /*
   * Create a datagram(UDP) socket in the internet domain
   */
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    PMAP_DEBUG_ERROR("socket() %s", strerror(errno));
    return -1;
  }

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 250000; // 250ms
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    PMAP_DEBUG_ERROR("setsockopt() %s", strerror(errno));
    return -1;
  }

  /**
   * To determine the external address, the client behind the NAT sends the
   * following UDP payload to port 5351 of the configured gateway address
   */
  npmp->sin_family = AF_INET; /* Internet Domain */
  npmp->sin_port = htons(NAT_PMP_SERVER_PORT);
  npmp->sin_addr.s_addr = gateway_ip;

  return sockfd;
}

/* -------------------------------------------- */

int pmap_npmp_getexip(pmap_field_t *pfield, char *external_ip, int esize,
                      char *error, int size) {

  int sockfd = 0;
  struct sockaddr_in npmp;
  struct sockaddr_in client;
  uint32_t ca_size = sizeof(client);
  int len;

  /* Setup socket */
  sockfd = _pmap_npm_setup_socket(&npmp, pfield->gateway_ip);
  if (sockfd < 0) {
    return 1; // caller should check errno value
  }

  nmpm_pkt_header npmp_hdr;
  npmp_hdr.version = NAT_PMP_VERSION;
  npmp_hdr.op_code = 0;

  PMAP_DEBUG_HEX_LOG(npmp_hdr, sizeof(nmpm_pkt_header),
                     "NAT-PMP REQUEST: =>>>\nLEN:%d\n",
                     (int)sizeof(nmpm_pkt_header));

  if (sendto(sockfd, &npmp_hdr, sizeof(npmp_hdr), 0, (struct sockaddr *)&npmp,
             sizeof(npmp)) < 0) {
    PMAP_DEBUG_ERROR("sendto() %s", strerror(errno));
    return 1; // caller should check errno value
  }

  nmpm_pkt_exip resp;
  memset(&resp, 0x00, sizeof(nmpm_pkt_exip));
  int retry = 2;
  while (true) {
    retry--;
    /* Receive NAT-PMP response */
    if ((len = recvfrom(sockfd, &resp, sizeof(nmpm_pkt_exip), 0,
                        (struct sockaddr *)&client, &ca_size)) > 0) {

      PMAP_DEBUG_HEX_LOG(&resp, sizeof(nmpm_pkt_exip),
                         "NAT-PMP RESPONSE: =>>>\nLEN:%d\n", len);

      if (len && client.sin_addr.s_addr == pfield->gateway_ip) {
        if (resp.res_code == 0) {
          strncpy(external_ip, pmap_ut_inet_ntoa(resp.external_ip), esize);
          return 0; // OK
        } else {
          resp.res_code = ntohs(resp.res_code);
          errno = NPMP_OK + resp.res_code;
          if (resp.res_code < sizeof(npmp_res_codes) && error != NULL) {
            memcpy(error, npmp_res_codes[resp.res_code], size);
          } else {
            /*
             * Fatal error described in RFC 6886 Page 16
             */
            memcpy(error, npmp_fatal_err, size);
          }
          return 1; // caller should check errno value
        }
      }

    } else {
      if (retry <= 0) {
        errno = ETIMEDOUT; // caller should check errno value
        break;
      }
    }
  }

  return 1; // caller should check errno value
}

/* -------------------------------------------- */

int pmap_npmp_addport(pmap_field_t *pfield, char *error, int size) {

  int sockfd = 0;
  struct sockaddr_in npmp;
  struct sockaddr_in client;
  uint32_t ca_size = sizeof(client);
  int len;

  /* Setup socket */
  sockfd = _pmap_npm_setup_socket(&npmp, pfield->gateway_ip);
  if (sockfd < 0) {
    return 1; // caller should check errno value
  }

  nmpm_pkt_req req_map;
  req_map.header.version = 0;

  if (strcmp(pfield->protocol, "UDP") == 0) {
    req_map.header.op_code = 1;
  } else if (strcmp(pfield->protocol, "TCP") == 0) {
    req_map.header.op_code = 2;
  } else {
    errno = EINVALIDPROT;
    return -2; // Protocol not supported
  }

  req_map.reserverd = 0;
  req_map.lifetime_sec = htonl(pfield->lifetime_sec);
  req_map.internal_port = htons(pfield->internal_port);
  req_map.external_port = htons(pfield->external_port);

  PMAP_DEBUG_HEX_LOG(&req_map, sizeof(nmpm_pkt_req),
                     "NAT-PMP REQUEST: =>>>\nLEN:%d\n",
                     (int)sizeof(nmpm_pkt_req));

  if (sendto(sockfd, &req_map, sizeof(nmpm_pkt_req), 0,
             (struct sockaddr *)&npmp, sizeof(npmp)) < 0) {
    PMAP_DEBUG_ERROR("sendto() %s", strerror(errno));
    return 1; // caller should check errno value
  }

  nmpm_pkt_resp resp;
  memset(&resp, 0x00, sizeof(nmpm_pkt_resp));
  int retry = 2;
  while (true) {
    retry--;
    /* Receive NAT-PMP response */
    if ((len = recvfrom(sockfd, &resp, sizeof(nmpm_pkt_resp), 0,
                        (struct sockaddr *)&client, &ca_size)) > 0) {

      PMAP_DEBUG_HEX_LOG(&resp, sizeof(nmpm_pkt_resp),
                         "NAT-PMP RESPONSE: =>>>\nLEN:%d\n", len);

      if (len && client.sin_addr.s_addr == pfield->gateway_ip) {
        if ((resp.header.op_code - 128) == req_map.header.op_code &&
            resp.res_code == 0) {
          pfield->external_port = ntohs(resp.external_port);
          pfield->internal_port = ntohs(resp.internal_port);
          pfield->lifetime_sec = ntohl(resp.lifetime_sec);
          return 0; // OK
        } else {
          resp.res_code = ntohs(resp.res_code);
          errno = NPMP_OK + resp.res_code;
          if (resp.res_code < sizeof(npmp_res_codes) && error != NULL) {
            memcpy(error, npmp_res_codes[resp.res_code], size);
          } else {
            /*
             * Fatal error described in RFC 6886 Page 16
             */
            memcpy(error, npmp_fatal_err, size);
          }
          return 1; // caller should check errno value
        }
      }
    } else {
      if (retry <= 0) {
        errno = ETIMEDOUT; // caller should check errno value
        break;
      }
    }
  }

  return 1; // caller should check errno value
}

int pmap_npmp_delport(pmap_field_t *pfield, char *error, int size) {

  pfield->lifetime_sec = 0; // Remove mapping
  return pmap_npmp_addport(pfield, error, size);
}
