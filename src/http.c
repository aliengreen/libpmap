/*
 *    http.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "buffer.h"
#include "http.h"
#include "pmap_debug.h"
#include "util.h"

extern uint8_t pmap_debug;

/* -------------------------------------------- */

/**
 * Create an HTTP request string with the specified method, hostname, port, and
 * path.
 *
 * This function creates an HTTP request string based on the provided HTTP
 * method, hostname, port, and path. It allocates a pbuffer_t object, builds the
 * request string, and returns it. The caller is responsible for freeing the
 * memory allocated for the pbuffer_t object by calling 'pbfr_destroy' function.
 *
 * @param method The HTTP method (e.g., "GET", "POST").
 * @param hostname The hostname of the server.
 * @param port The port number to connect to.
 * @param path The path of the HTTP request.
 * @return A pbuffer_t object containing the HTTP request, or NULL on error.
 */
pbuffer_t *pmap_http_create(const char *method, const char *hostname, int port,
                            char *path) {

  pbuffer_t *pbfr = pbfr_create(PBUFFER_DEFLEN);

  const char *request = "%s %s HTTP/1.1\r\n";
  const char *requestSlash = "%s /%s HTTP/1.1\r\n";

  if (NULL != pbfr) {
    if (NULL != path && *path == '/') {
      pbfr_add(pbfr, request, method, path);
    } else {
      pbfr_add(pbfr, requestSlash, method, path);
    }

    pbfr_add(pbfr, "Host: %s:%d\r\n", hostname, port);
  }

  return pbfr;
}

/**
 * Establish a non-blocking TCP connection to a remote host with the specified
 * hostname and port.
 *
 * This function creates a non-blocking TCP socket and attempts to connect to a
 * remote host identified by its hostname and port number. If the connection is
 * established, it returns the socket file descriptor. If the connection is in
 * progress, it waits for a certain timeout. If a timeout occurs or an error is
 * encountered, the function returns -1.
 *
 * @param hostname The hostname or IP address of the remote host.
 * @param port The port number to connect to on the remote host.
 * @return The socket file descriptor if the connection is established, or -1 on
 * error or timeout.
 */
int pmap_http_connect(const char *hostname, int port) {

  struct hostent *server;
  struct sockaddr_in server_addr;
  int sockfd;
  fd_set fdset;
  struct timeval tv;
  int ret = 0;

  // Create a socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    PMAP_DEBUG_ERROR("socket() %s", strerror(errno));
    return -1;
  }

  // Get server information by hostname
  server = gethostbyname(hostname);
  if (server == NULL) {
    PMAP_DEBUG_ERROR("Host not found");
    return -1;
  }

  // Initialize the server address structure
  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr,
        server->h_length);
  server_addr.sin_port = htons(port);

  fcntl(sockfd, F_SETFL, O_NONBLOCK);

  // Connect to the server
  ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (ret == 0) { // Connected
    return sockfd;
  } else if (ret == -1) { // Connection in progress

    FD_ZERO(&fdset);
    FD_SET(sockfd, &fdset);
    tv.tv_sec = PMAP_DEFAULT_WAIT_TIMEOUT; /* PMAP_DEFAULT_WAIT_TIMEOUT in
                                              second timeout */
    tv.tv_usec = 0;

    int ret = select(sockfd + 1, NULL, &fdset, NULL, &tv);
    if (ret > 0) {
      if (FD_ISSET(sockfd, &fdset)) {
        // The socket is now connected
        PMAP_DEBUG_LOG("Connected\n");
        return sockfd;
      } else {
        // Timeout occurred
        PMAP_DEBUG_LOG("Connection timeout\n");
        close(sockfd);
      }
    } else if (ret == 0) {
      // Timeout occurred
      PMAP_DEBUG_LOG("Connection timeout (2)\n");
      close(sockfd);
    } else {
      PMAP_DEBUG_ERROR("select() %s", strerror(errno));
    }

  } else {
    PMAP_DEBUG_ERROR("connect() %s", strerror(errno));
  }

  return -1;
}

/* -------------------------------------------- */

/**
 * Send an HTTP request to a remote host and receive the response.
 *
 * This function establishes a connection to a remote host specified by its
 * hostname and port, sends an HTTP request contained in a pbuffer_t object, and
 * receives the HTTP response. It uses a non-blocking socket to handle the
 * connection and incorporates timeouts. The received response is stored in a
 * new pbuffer_t object, and the HTTP status code (if available) is returned
 * through the http_status pointer.
 *
 *
 * @param hostname The hostname or IP address of the remote host.
 * @param port The port number to connect to on the remote host.
 * @param pbfr A pbuffer_t object containing the HTTP request to be sent.
 * @param http_status A pointer to an integer where the HTTP status code will be
 * stored.
 * @return A pbuffer_t object containing the HTTP response, or NULL on error.
 * The caller is responsible for freeing the memory allocated for the pbuffer_t
 * object by calling 'pbfr_destroy' function.
 */
pbuffer_t *pmap_http_req(const char *hostname, int port, pbuffer_t *pbfr,
                         int *http_status) {

  int totall_bytes_received = 0;
  int remain_buffer_len = 0;

  int sockfd = pmap_http_connect(hostname, port);
  if (sockfd == -1) {
    PMAP_DEBUG_ERROR("Error connection %s", strerror(errno));
    return NULL;
  }

  PMAP_DEBUG_LOG("REQUEST: =>>>\n%s\n", pbfr->buffer);

  if (pmap_debug) {
    PMAP_RUNTIME_LOG("REQUEST: =>>>\n%s\n", pbfr->buffer);
  }

  // Send the request
  int n = write(sockfd, pbfr->buffer, pbfr->offset);
  if (n < 0) {
    PMAP_DEBUG_ERROR("write() %s", strerror(errno));
    return NULL;
  }

  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(sockfd, &read_fds);

  pbuffer_t *pbfr_recv = pbfr_create(PBUFFER_DEFLEN);

  remain_buffer_len = pbfr_recv->size;

  if (http_status != NULL) {
    *http_status = 0;
  }

  while (1) {

    struct timeval timeout;
    timeout.tv_sec = 2;       // 1 sec
    timeout.tv_usec = 100000; // 100 milliseconds

    int ready = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

    if (ready == -1) {
      PMAP_DEBUG_ERROR("select() %s", strerror(errno));
      break;
    } else if (ready == 0) {
      // No data available, continue or perform other tasks
      PMAP_DEBUG_LOG("No data available.\n");
      break;
    } else {
      if (FD_ISSET(sockfd, &read_fds)) {

        // Data is available to read
        ssize_t bytes_received =
            recv(sockfd, pbfr_recv->buffer + totall_bytes_received,
                 remain_buffer_len, 0);

        if (bytes_received <= 0) {
          // Connection closed
          char status[16];
          pbfr_recv->buffer[totall_bytes_received] = '\0';
          if ((pmap_ut_substr(" ", " ", pbfr_recv->buffer, status,
                              sizeof(status))) == 0) {
            if (http_status != NULL) {
              *http_status = atoi(status);
            }
          }

          PMAP_DEBUG_LOG("RESPONSE: =>>>\n%s\n", pbfr_recv->buffer);
          if (pmap_debug) {
            PMAP_RUNTIME_LOG("RESPONSE: =>>>\n%s\n", pbfr_recv->buffer);
          }

          break;
        } else {
          totall_bytes_received += bytes_received;
          remain_buffer_len -= bytes_received;
          // printf("remain_buffer_len: %d\n", remain_buffer_len);
        }
      }
    }
  }

  /* Release buffer */
  // pbfr_destroy(pbfr_recv);

  /* Close socket connection */
  close(sockfd);

  return pbfr_recv;
}
