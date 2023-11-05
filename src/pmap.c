/*
 *    pmap.c
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
#include "pmap.h"
#include "pmap_debug.h"
#include "upnp_msg.h"
#include "util.h"

uint8_t pmap_debug = false;

/* -------------------------------------------- */

/**
 * Set the debug mode for the pmap lib.
 *
 * This function allows you to enable or disable debugging output for the
 * network packet. Debugging output is managed by a boolean value where
 * 'false' means debugging is disabled, and 'true' means debugging is enabled.
 *
 * @param debug A boolean value indicating whether to enable or disable
 * debugging. Use 'false' to disable debugging and 'true' to enable it.
 */
void pmap_http_set_debug(uint8_t debug) {

  /* This global variable - false mean disable debug, true enable */
  pmap_debug = debug;
}

/* -------------------------------------------- */

/**
 * Discover UPnP devices in the local network and filter Internet Gateway
 * Devices (IGDs).
 *
 * This function initiates the discovery of UPnP devices within the local
 * network by broadcasting an M-SEARCH message via UDP to the multicast address
 * 239.255.255.250. The discovered devices, including Internet Gateway Devices
 * (IGDs) and other devices, are filtered and identified based on their
 * response.
 *
 * @param urls A pointer to a pointer for storing the list of discovered and
 * filtered UPnP devices.
 * @param only_igds Set to 1 to filter and retrieve only Internet Gateway
 * Devices (IGDs).
 * @return 0 on success, -1 on failure.
 */
int pmap_list_upnp(pmap_url_comp_t **urls, uint8_t only_igds) {

  int sockfd = 0;
  struct sockaddr_in igds;
  struct sockaddr_in client;
  uint32_t ca_size = sizeof(client);
  int len;

  pmap_url_comp_t *url_comp = *urls = NULL;
  pmap_url_comp_t *head = NULL;

  /*
   * Create a datagram(UDP) socket in the internet domain
   */
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    PMAP_DEBUG_ERROR("socket() %s", strerror(errno));
    return -1;
  }

  struct timeval tv;
  tv.tv_sec = PMAP_DEFAULT_WAIT_TIMEOUT;
  tv.tv_usec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    PMAP_DEBUG_ERROR("setsockopt() %s", strerror(errno));
    return -1;
  }

  /**
   * This SSDP discovery service for UPnP is a UDP service that responds on port
   * 1900 and can be enumerated by broadcasting an M-SEARCH message via the
   * multicast address 239.255.255.250.
   */
  igds.sin_family = AF_INET; /* Internet Domain */
  igds.sin_port = htons(1900);
  igds.sin_addr.s_addr = inet_addr("239.255.255.250");

  PMAP_DEBUG_LOG("M-SEARCH REQUEST: =>>>\n%s\n", m_search);
  if (pmap_debug) {
    PMAP_RUNTIME_LOG("M-SEARCH REQUEST: =>>>\n%s\n", m_search);
  }

  /* Send the m-search message to the server(s) */
  if (sendto(sockfd, m_search, (strlen(m_search)), 0, (struct sockaddr *)&igds,
             sizeof(igds)) < 0) {
    PMAP_DEBUG_ERROR("sendto() %s", strerror(errno));
    return -1;
  }
  pbuffer_t *pbfr = pbfr_create(1024);

  while (true) {

    /* Receive M-SEARCH response */
    if ((len = recvfrom(sockfd, pbfr->buffer, pbfr->size, 0,
                        (struct sockaddr *)&client, &ca_size)) > 0) {

      pbfr->buffer[len] = 0;

      PMAP_DEBUG_LOG("M-SEARCH RESPONSE: =>>>\n%s\n", pbfr->buffer);
      if (pmap_debug) {
        PMAP_RUNTIME_LOG("M-SEARCH RESPONSE: =>>>\n%s\n", pbfr->buffer);
      }

      char tmp[128];
      if (pmap_ut_substr("LOCATION:", "\r\n", pbfr->buffer, tmp, sizeof(tmp)) ==
          0) {

        url_comp = pmap_ut_parse_url(tmp);
        if (NULL != url_comp) {

          /**
           * Here we check IGD by calling HTTP get request to see device type.
           * NOTE: this is time consuming process !
           */
          if (only_igds) {
            int http_status;
            memset(tmp, 0x00, sizeof(tmp));
            if ((http_status = pmap_req_ctrlurl(url_comp, tmp, sizeof(tmp))) !=
                0) {
              /* Cleanup */
              pmap_ut_free_url(url_comp);
              continue;
            } else {
              int ctrl_url_len = strlen(tmp);
              if (ctrl_url_len) {
                url_comp->crtl_url = malloc(ctrl_url_len + 1);
                strncpy(url_comp->crtl_url, tmp, ctrl_url_len);
                url_comp->crtl_url[ctrl_url_len] = 0;
              } else {
                /* Cleanup */
                pmap_ut_free_url(url_comp);
                continue;
              }
            }
          }

          int found = 0;
          for (pmap_url_comp_t *ucmp = *urls; ucmp != NULL; ucmp = ucmp->next) {
            if (PMAP_COMPARE_URLCOMP(ucmp, url_comp)) {
              found = 1;
            }
          }

          if (!found) {
            if (head != NULL) {
              head->next = url_comp;
            }
            head = url_comp;
            if (*urls == NULL) {
              *urls = head;
            }
          } else {
            /* Cleanup */
            pmap_ut_free_url(url_comp);
          }
        } else {
          PMAP_DEBUG_ERROR("Can't parse URL [%s]", tmp);
          return -1;
        }
      }

    } else {
      /* Timeout, nothing received  */
      break;
    }
  }

  pbfr_destroy(pbfr);

  return 0;
}

/**
 * Discover and filter Internet Gateway Devices (IGDs) in the local network.
 *
 * This function initiates the discovery of Internet Gateway Devices (IGDs)
 * within the local network by leveraging the UPnP device discovery process. It
 * filters and retrieves only IGDs from the list of discovered UPnP devices.
 *
 * @param urls A pointer to a pointer for storing the list of discovered and
 * filtered IGDs.
 * @return 0 on success, -1 on failure.
 */
int pmap_list_igd(pmap_url_comp_t **urls) {
  return pmap_list_upnp(urls, PMAP_UPNP_LIST_IGD);
}

/**
 * Free a linked list of pmap_url_comp_t structures and associated memory.
 *
 * This function is responsible for freeing a linked list of pmap_url_comp_t
 * structures and the associated memory. It iterates through the linked list,
 * starting from the provided head node, and deallocates the memory for each
 * structure and its components. It ensures proper cleanup of the entire list.
 *
 * @param urls A pointer to the head of the linked list of pmap_url_comp_t
 * structures.
 */
void pmap_list_free(pmap_url_comp_t *urls) {

  pmap_url_comp_t *head = urls;
  pmap_url_comp_t *tmp;
  while (head != NULL) {
    tmp = head;
    head = head->next;
    pmap_ut_free_url(tmp);
  }
}

/**
 * Request the control URL from a UPnP device and extract it for Internet
 * Gateway Device (IGD) identification.
 *
 * This function sends an HTTP GET request to the specified UPnP device to
 * retrieve its device description (rootDesc.xml). It then extracts the device
 * type and the control URL for further use in IGD identification.
 *
 * @param ucmp A pointer to a structure containing the URL components of the
 * UPnP device.
 * @param ctrl_url A buffer to store the control URL if found.
 * @param size The size of the control URL buffer.
 * @return 0 on success (HTTP status 200), 1 on failure.
 */
int pmap_req_ctrlurl(pmap_url_comp_t *ucmp, char *ctrl_url, int size) {

  pbuffer_t *pbfr_recv = NULL;
  int ret = 0;
  int http_status = 0;

  pbuffer_t *pbfr_tmp = pbfr_create(128);
  if (NULL == pbfr_tmp) {
    return http_status;
  }

  /* Get rootDesc.xml from device to extract control endpoint */
  pbfr_recv = pmap_http_get(ucmp->host, ucmp->port, ucmp->path, &http_status);

  if (NULL == pbfr_recv) {
    goto cleanup;
  }

  /* Extract Device Type from XML */
  if ((ret = pmap_ut_substr("<deviceType>", "</deviceType>", pbfr_recv->buffer,
                            pbfr_tmp->buffer, pbfr_tmp->size)) != 0) {
    goto cleanup;
  }

  /**
   * Device type should be urn:schemas-upnp-org:device:InternetGatewayDevice:1
   * string
   */
  if (strcmp(pbfr_tmp->buffer,
             "urn:schemas-upnp-org:device:InternetGatewayDevice:1") != 0) {
    goto cleanup;
  }

  PMAP_DEBUG_LOG("InternetGatewayDevice=[%s]\n", pbfr_tmp->buffer);

  char *start = strstr(pbfr_recv->buffer,
                       "urn:schemas-upnp-org:service:WANIPConnection:1");
  if (start) {
    if ((ret = pmap_ut_substr("<controlURL>", "</controlURL>", start,
                              pbfr_tmp->buffer, pbfr_tmp->size)) == 0) {
      strncpy(ctrl_url, pbfr_tmp->buffer, size);
      ctrl_url[size] = 0;
    }
  }

cleanup:
  pbfr_destroy(pbfr_recv);
  pbfr_destroy(pbfr_tmp);

  return (http_status == 200) ? 0 : 1;
}

/**
 * Add a port mapping using UPnP.
 *
 * This function is used to add a port mapping on a UPnP-enabled device,
 * typically a router or gateway, in the local network. It sends the UPnP action
 * to add the port mapping and checks the HTTP response status for success or
 * failure. If the operation is unsuccessful, it can provide an error
 * description if available.
 *
 * @param pfield A pointer to a `pmap_field_t` structure containing the details
 * of the port mapping to be added.
 * @param error A character array where an error description will be stored in
 * case of failure.
 * @param size The size of the `error` character array.
 *
 * @return An integer representing the result of the operation.
 *         - If the port mapping is added successfully, it returns 0.
 *         - If the operation fails or an error occurs, it returns 1.
 *
 * @note The `pfield` parameter should be filled with the necessary information
 * for the port mapping, including external and internal port numbers, IP
 * addresses, and the protocol.
 * @note If the HTTP response status code is not 200 (OK), the function attempts
 * to extract an error description from the response and stores it in the
 * `error` parameter if there's enough space.
 */
int pmap_upnp_addport(pmap_field_t *pfield, char *error, int size) {

  int http_status = 0;

  pbuffer_t *pbfr_recv =
      pmap_upnp_action(PMAP_UPNP_ACTION_ADDPORT, pfield, &http_status);

  if (pbfr_recv) {
    if (http_status != 200) {
      pmap_ut_substr("<errorDescription>", "</errorDescription>",
                     pbfr_recv->buffer, error, size);
    }
    pbfr_destroy(pbfr_recv);
  }

  return (http_status == 200) ? 0 : 1;
}

/**
 * Delete a port mapping using UPnP.
 *
 * This function is used to remove a previously configured port mapping on a
 * UPnP-enabled device. It sends the UPnP action to delete the port
 * mapping and checks the HTTP response status for success or failure. If the
 * operation is unsuccessful, it can provide an error description if available.
 *
 * @param pfield A pointer to a `pmap_field_t` structure containing the details
 * of the port mapping to be deleted.
 * @param error A character array where an error description will be stored in
 * case of failure.
 * @param size The size of the `error` character array.
 *
 * @return An integer representing the result of the operation.
 *         - If the port mapping is deleted successfully, it returns 0.
 *         - If the operation fails or an error occurs, it returns 1.
 *
 * @note The `pfield` parameter should be filled with the necessary information
 * for the port mapping to be deleted, including external and internal port
 * numbers, IP addresses, and the protocol.
 * @note If the HTTP response status code is not 200 (OK), the function attempts
 * to extract an error description from the response and stores it in the
 * `error` parameter if there's enough space.
 */
int pmap_upnp_delport(pmap_field_t *pfield, char *error, int size) {

  int http_status = 0;

  pbuffer_t *pbfr_recv =
      pmap_upnp_action(PMAP_UPNP_ACTION_DELPORT, pfield, &http_status);

  if (pbfr_recv) {
    if (http_status != 200) {
      pmap_ut_substr("<errorDescription>", "</errorDescription>",
                     pbfr_recv->buffer, error, size);
    }
    pbfr_destroy(pbfr_recv);
  }

  return (http_status == 200) ? 0 : 1;
}

/**
 * Retrieve the external IP address of a UPnP-enabled device.
 *
 * This function sends a UPnP action to query the external IP address of a
 * UPnP-enabled device, within the local network.
 * It checks the HTTP response status for success or failure and extracts the
 * external IP address if the operation is successful. In case of failure, it
 * can provide an error description.
 *
 * @param pfield A pointer to a `pmap_field_t` structure containing the
 * necessary information for the UPnP action.
 * @param external_ip A character array where the external IP address will be
 * stored if the operation is successful.
 * @param esize The size of the `external_ip` character array.
 * @param error A character array where an error description will be stored in
 * case of failure.
 * @param size The size of the `error` character array.
 *
 * @return An integer representing the result of the operation.
 *         - If the external IP address is successfully retrieved, it returns 0,
 * and the `external_ip` parameter contains the address.
 *         - If the operation fails or an error occurs, it returns 1, and the
 * `error` parameter may contain a description of the error.
 *
 * @note The `pfield` parameter should be filled with the necessary information
 * for the UPnP action, but the exact content may vary depending on the UPnP
 * device.
 * @note If the HTTP response status code is 200 (OK), the function extracts the
 * external IP address from the response and stores it in the `external_ip`
 * parameter.
 * @note If the HTTP response status code is not 200, the function attempts to
 * extract an error description from the response and stores it in the `error`
 * parameter if there's enough space.
 */
int pmap_upnp_getexip(pmap_field_t *pfield, char *external_ip, int esize,
                      char *error, int size) {

  int http_status = 0;
  pbuffer_t *pbfr_recv =
      pmap_upnp_action(PMAP_UPNP_ACTION_GETEXTIP, pfield, &http_status);

  if (pbfr_recv) {
    if (http_status == 200) {
      pmap_ut_substr("<NewExternalIPAddress>", "</NewExternalIPAddress>",
                     pbfr_recv->buffer, external_ip, esize);
    } else {
      pmap_ut_substr("<errorDescription>", "</errorDescription>",
                     pbfr_recv->buffer, error, size);
    }
    pbfr_destroy(pbfr_recv);
  }

  return (http_status == 200) ? 0 : 1;
}

/**
 * Perform a UPnP action on a specific UPnP-enabled device.
 *
 * This function initiates a UPnP action on a target UPnP-enabled device, such
 * as a router or gateway. It first searches for the device's control URL, sends
 * the SOAP request, and receives the response.
 *
 * @param action An integer representing the type of UPnP action to be performed
 * (e.g., add port mapping, delete port mapping, get external IP).
 * @param pfield A pointer to a `pmap_field_t` structure containing the
 * necessary information for the UPnP action.
 * @param http_status A pointer to an integer where the HTTP response status
 * code will be stored.
 *
 * @return A pointer to a `pbuffer_t` structure containing the HTTP response
 * from the UPnP-enabled device.
 *         - If successful, this pointer will point to the response data.
 *         - If the operation fails or an error occurs, it returns NULL.
 *
 * @note The `action` parameter specifies the type of UPnP action to perform
 * (e.g., `PMAP_UPNP_ACTION_ADDPORT`, `PMAP_UPNP_ACTION_DELPORT`, or
 * `PMAP_UPNP_ACTION_GETEXTIP`).
 * @note The `pfield` parameter is populated with the information required for
 * the specific UPnP action, including port mapping details or external IP
 * retrieval.
 * @note The `http_status` parameter is used to store the HTTP response status
 * code, which can be checked to determine the success or failure of the UPnP
 * action.
 * @note The allocated `pbuffer_t` structures should be destroyed by the caller
 * when no longer needed to prevent memory leaks.
 */
pbuffer_t *pmap_upnp_action(int action, pmap_field_t *pfield,
                            int *http_status) {

  pmap_url_comp_t *urls;
  pbuffer_t *pbfr_rcv = NULL;

  pbuffer_t *pbfr_tmp = pbfr_create(128);
  if (NULL == pbfr_tmp) {
    return NULL;
  }

  /* Get list of all UPnP devices (M-SEARCH) */
  pmap_list_upnp(&urls, 0);

  for (pmap_url_comp_t *ucmp = urls; ucmp != NULL; ucmp = ucmp->next) {

    /* Skip other UPnP devices */
    if (strcmp(ucmp->host, pmap_ut_inet_ntoa(pfield->gateway_ip)) != 0) {
      continue;
    }

    /* The 'controlURL' value will be stored in the pbfr_tmp buffer. */
    if ((*http_status =
             pmap_req_ctrlurl(ucmp, pbfr_tmp->buffer, pbfr_tmp->size)) == 0) {

      PMAP_DEBUG_LOG("[controlURL=%s]\n", pbfr_tmp->buffer);

      pbuffer_t *pbfr_body = pbfr_create(1024);
      char *soap_header = NULL;
      if (action == PMAP_UPNP_ACTION_ADDPORT) {
        soap_header = "SOAPAction: "
                      "\"urn:schemas-upnp-org:service:WANIPConnection:1#"
                      "AddPortMapping\"\r\n";
        pbfr_add(pbfr_body, soap_action_add, pfield->external_port,
                 pfield->protocol, pfield->internal_port,
                 pmap_ut_inet_ntoa(pfield->internal_ip), pfield->lifetime_sec);
      } else if (action == PMAP_UPNP_ACTION_DELPORT) {
        soap_header = "SOAPAction: "
                      "\"urn:schemas-upnp-org:service:WANIPConnection:1#"
                      "DeletePortMapping\"\r\n";
        pbfr_add(pbfr_body, soap_action_del, pfield->external_port,
                 pfield->protocol);
      } else if (action == PMAP_UPNP_ACTION_GETEXTIP) {
        soap_header = "SOAPAction: "
                      "\"urn:schemas-upnp-org:service:WANIPConnection:1#"
                      "GetExternalIPAddress\"\r\n";
        pbfr_add(pbfr_body, soap_action_getextip);
      }

      if (NULL != pbfr_rcv) {
        pbfr_destroy(pbfr_rcv);
      }

      pbfr_rcv = pmap_http_post(ucmp->host, ucmp->port, pbfr_tmp->buffer,
                                soap_header, pbfr_body, http_status);

      pbfr_destroy(pbfr_body);
      PMAP_DEBUG_LOG("[HTTP Status Code=%d]\n", *http_status);
      break;
    }
  }

  /* Destroy components allocated by 'pmap_get_ids' function */
  pmap_list_free(urls);
  //  pbfr_destroy(pbfr_rcv);
  pbfr_destroy(pbfr_tmp);

  return pbfr_rcv;
}
