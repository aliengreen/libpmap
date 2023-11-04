//
//  main.c
//  libmap
//
//  Created by lashadolidze on 29.10.23.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pmap.h"
#include "upnp_msg.h"

extern char error_description[100];

int main(int argc, const char * argv[]) {
    
 
        int port_map = 6595;
        int ret = 0;
//    printf("LENGTH: %d\n", strlen(soap_action_add));
//        if ((ret = pmap_upnp_addport(port_map, port_map, "TCP", "192.168.2.2",
//                                     "192.168.2.1", 0)) == 0) {
//          printf("Add mapping to TCP port: %d\n", port_map);
//        } else {
//          printf("Error adding mapping to TCP port: %d, error code=%d,%s\n", port_map,
//                 ret, error_description);
//        }

         if ((ret = pmap_upnp_delport(port_map, "TCP", "192.168.2.2",
                                      "192.168.2.1")) == 0) {
           printf("Delete mapping TCP port: %d\n", port_map);
         } else {
             printf("Error deleting mapping to TCP port: %d, error code=%d,%s\n", port_map,
                              ret, error_description);
         }
//            char ip[64];
//             if ((ret = pmap_upnp_getexip("192.168.2.1", ip, sizeof(ip))) == 0) {
//               printf("External IP: %s\n", ip);
//             } else {
//               printf("Error getting external ip\n");
//             }
    

    
    return 0;
}
