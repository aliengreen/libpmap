/*
 *    main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifndef WIN32
#include <unistd.h>
#else
#include "xgetopt.h"
#endif

#include "pmap_npmp.h"
#include "pmap_upnp.h"

#define OP_PROTOCOL_NAT_PMP 1
#define OP_PROTOCOL_UPNP 2
#define OP_LIST 3

static const char err_arg_missing[] = "Argument(s) missing !\n";

int opt_debug = 0;
int debug_level = 0; // NO_DEBUG;

int pmap_upnp(int argc, char **argv);
void usage(char *progname);
void print_list();
int print_upnp_exip(int argc, char **argv);
int addport_upnp(int argc, char **argv);
int delport_upnp(int argc, char **argv);

int print_npmp_exip(int argc, char **argv);
int addport_npmp(int argc, char **argv);
int delport_npmp(int argc, char **argv);

int main(int argc, char *argv[]) {
  int ret;
  int operation = 0;
  int action = 0;

#ifdef WIN32
  WSADATA wsa_data;
  ret = WSAStartup(MAKEWORD(2, 0), &wsa_data);
  ERROR_GOTO(ret != 0, "WSAStartup() failed", error);
#endif

  while ((ret = getopt(argc, argv, "adhpulev")) != EOF) {
    switch (ret) {

    case 'p':
      operation = OP_PROTOCOL_NAT_PMP;
      break;

    case 'u':
      operation = OP_PROTOCOL_UPNP;
      break;

    case 'a':
      action = 1; // Add port
      break;

    case 'd':
      action = 2; // Delete port
      break;

    case 'e':
      action = 3; // Get external IP
      break;

    case 'v':
      debug_level++;
      break;

    case 'l':
      operation = OP_LIST;
      break;

    case 'h':
      /* fall through */
    default:
      goto error;
    }
  }

  ret = 0;

  /* Set debug  */
  pmap_set_debug(debug_level);

  if (operation == OP_LIST) {
    print_list();
  } else if (operation == OP_PROTOCOL_NAT_PMP) {

    if (action == 0) {
      printf("-a, -d  or -e options should be specified\n");
    } else if (action == 1) {
      if (addport_npmp(argc, argv) != 0) {
        usage(argv[0]);
      }
    } else if (action == 2) {
      if (delport_npmp(argc, argv) != 0) {
        usage(argv[0]);
      }
    } else if (action == 3) {
      if (print_npmp_exip(argc, argv) != 0) {
        usage(argv[0]);
      }
    }

  } else if (operation == OP_PROTOCOL_UPNP) {

    if (action == 0) {
      printf("-a, -d  or -e options should be specified\n");
    } else if (action == 1) {
      if (addport_upnp(argc, argv) != 0) {
        usage(argv[0]);
      }
    } else if (action == 2) {
      if (delport_upnp(argc, argv) != 0) {
        usage(argv[0]);
      }
    } else if (action == 3) {
      if (print_upnp_exip(argc, argv) != 0) {
        usage(argv[0]);
      }
    }

  } else {
    usage(argv[0]);
  }

#ifdef WIN32
  WSACleanup();
#endif

  return ret;

error:
  usage(argv[0]);

  exit(1);
}

void usage(char *progname) {
  printf("usage: %s < -p | -u | -l > < -a | -d | -e > <args>\n", progname);
  printf("  -p    Using NAT-PMP protocol for port mapping\n"
         "        <args>: <external port> <my_ip_v4> <gateway_ip_v4> "
         "<protocol> <lifetime>\n"
         "  -u    Using UPnP protocol for port mapping\n"
         "        <args>: <external port> <my_ip_v4> <gateway_ip_v4> "
         "<protocol> <lifetime>\n"
         "  -a    Add port mapping\n"
         "  -d    Delete port mapping\n"
         "  -e    Get external IP address\n"
         "  -l    Print list of available IGDs (UPnP)\n"
         "  -v    show request => response debug output\n"
         "  -h    show this help and exit\n"
         "Example 1: %s -l\n"
         "Example 2: %s -u -a 6568 192.168.1.7 192.168.1.1 TCP 7200\n"
         "Example 3: %s -u -d 6568 192.168.1.1 TCP\n"
         "Example 4: %s -u -e 192.168.1.1\n",
         progname, progname, progname, progname);
}

/* -------------------------------------------- */

void print_list() {

  pmap_url_comp_t *urls;

  printf("Request...\n");
  pmap_list_upnp(&urls, PMAP_UPNP_LIST_ALL);
  printf("-----------------------------------------------------------\n");
  printf("Host\t\t\tPath\t\tControl URL\n");
  printf("-----------------------------------------------------------\n");
  for (pmap_url_comp_t *ucmp = urls; ucmp != NULL; ucmp = ucmp->next) {
    printf("%s:%d\t%s\t%s\n", ucmp->host, ucmp->port, ucmp->path,
           ucmp->crtl_url);
  }
  printf("-----------------------------------------------------------\n");

  pmap_list_free(urls);
}

/* -------------------------------------------- */

int print_upnp_exip(int argc, char **argv) {

  int count = argc - optind;
  if (count < 1) {
    fprintf(stderr, err_arg_missing);
    return 1;
  }

  char *gateway_ip = argv[optind];

  int ret = 0;
  char external_ip[16];
  char error_desc[64];
  pmap_field_t pfield;
  pfield.gateway_ip = inet_addr(gateway_ip);
  printf("Request...\n");
  if ((ret = pmap_upnp_getexip(&pfield, external_ip, sizeof(external_ip),
                               error_desc, sizeof(error_desc))) == 0) {
    printf("External IP=[%s]\n", external_ip);
  } else {
    printf("Error getting external IP, error code=%d [%s]\n", errno,
           error_desc);
  }

  return 0;
}

/* -------------------------------------------- */

int addport_upnp(int argc, char **argv) {

  int count = argc - optind;
  if (count < 4) {
    fprintf(stderr, err_arg_missing);
    return 1;
  }

  int port = atoi(argv[optind]);
  char *my_ip = argv[optind + 1];
  char *gateway_ip = argv[optind + 2];
  char *protocol = argv[optind + 3];
  int lifetime = 0;
  if (count == 5) {
    lifetime = atoi(argv[optind + 4]);
  }

  int ret = 0;
  char error_desc[64];
  pmap_field_t pfield;
  pfield.external_port = port;
  pfield.internal_port = port;
  pfield.lifetime_sec = lifetime;

  pfield.internal_ip = inet_addr(my_ip);
  pfield.gateway_ip = inet_addr(gateway_ip);
  strncpy(pfield.protocol, protocol, sizeof(pfield.protocol));

  printf("Request...\n");
  if ((ret = pmap_upnp_addport(&pfield, error_desc, sizeof(error_desc))) == 0) {

    printf("Add port mapping to [%s => %d] lifetime=%d secs%s\n", protocol,
           port, lifetime, (lifetime == 0) ? " (no expiration)" : "");
  } else {
    printf("Error adding port mapping, error code=%d [%s]\n", errno,
           error_desc);
  }

  return 0;
}

/* -------------------------------------------- */

int delport_upnp(int argc, char **argv) {

  int count = argc - optind;
  if (count < 2) {
    fprintf(stderr, err_arg_missing);
    return 1;
  }
  int port = atoi(argv[optind]);
  char *gateway_ip = argv[optind + 1];
  char *protocol = argv[optind + 2];

  int ret = 0;
  char error_desc[64];
  pmap_field_t pfield;
  pfield.external_port = port;
  pfield.internal_port = port;
  pfield.gateway_ip = inet_addr(gateway_ip);

  strncpy(pfield.protocol, protocol, sizeof(pfield.protocol));

  printf("Request...\n");
  if ((ret = pmap_upnp_delport(&pfield, error_desc, sizeof(error_desc))) == 0) {

    printf("Delete port mapping to [%s => %d]\n", protocol, port);
  } else {
    printf("Error deleting port mapping, error code=%d [%s]\n", errno,
           error_desc);
  }

  return 0;
}

/* -------------------------------------------- */

int print_npmp_exip(int argc, char **argv) {

  int count = argc - optind;
  if (count < 1) {
    fprintf(stderr, err_arg_missing);
    return 1;
  }

  char *gateway_ip = argv[optind];

  int ret = 0;
  char external_ip[16];
  char error_desc[64];
  pmap_field_t pfield;
  pfield.gateway_ip = inet_addr(gateway_ip);
  printf("Request...\n");
  if ((ret = pmap_npmp_getexip(&pfield, external_ip, sizeof(external_ip),
                               error_desc, sizeof(error_desc))) == 0) {
    printf("External IP=[%s]\n", external_ip);
  } else {
    printf("Error getting external IP, error code=%d [%s]\n", errno,
           error_desc);
  }

  return 0;
}

/* -------------------------------------------- */

int addport_npmp(int argc, char **argv) {

  int count = argc - optind;
  if (count < 4) {
    fprintf(stderr, err_arg_missing);
    return 1;
  }

  int port = atoi(argv[optind]);
  char *my_ip = argv[optind + 1];
  char *gateway_ip = argv[optind + 2];
  char *protocol = argv[optind + 3];
  int lifetime = 0;
  if (count == 5) {
    lifetime = atoi(argv[optind + 4]);
  }

  int ret = 0;
  char error_desc[64];
  pmap_field_t pfield;
  pfield.external_port = port;
  pfield.internal_port = port;
  pfield.lifetime_sec = lifetime;

  pfield.internal_ip = inet_addr(my_ip);
  pfield.gateway_ip = inet_addr(gateway_ip);
  strncpy(pfield.protocol, protocol, sizeof(pfield.protocol));

  printf("Request...\n");
  if ((ret = pmap_npmp_addport(&pfield, error_desc, sizeof(error_desc))) == 0) {

    printf("Add port mapping to [%s => %d] lifetime=%d secs%s\n", protocol,
           pfield.external_port, pfield.lifetime_sec,
           (lifetime == 0) ? " (no expiration)" : "");
  } else {
    printf("Error adding port mapping, error code=%d [%s]\n", errno,
           error_desc);
  }

  return 0;
}

/* -------------------------------------------- */

int delport_npmp(int argc, char **argv) {

  int count = argc - optind;
  if (count < 2) {
    fprintf(stderr, err_arg_missing);
    return 1;
  }
  int port = atoi(argv[optind]);
  char *gateway_ip = argv[optind + 1];
  char *protocol = argv[optind + 2];

  int ret = 0;
  char error_desc[64];
  pmap_field_t pfield;
  pfield.external_port = port;
  pfield.internal_port = port;
  pfield.gateway_ip = inet_addr(gateway_ip);

  strncpy(pfield.protocol, protocol, sizeof(pfield.protocol));

  printf("Request...\n");
  if ((ret = pmap_npmp_delport(&pfield, error_desc, sizeof(error_desc))) == 0) {

    printf("Delete port mapping to [%s => %d]\n", protocol, port);
  } else {
    printf("Error deleting port mapping, error code=%d [%s]\n", errno,
           error_desc);
  }

  return 0;
}
