//
//  main.c
//  libmap
//
//  Created by lashadolidze on 29.10.23.
//

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pmap_upnp.h"
#include "pmap_npmp.h"
#include "upnp_msg.h"

extern char error_description[100];

int main(int argc, const char * argv[]) {
    
 
    pmap_detect_npmp(inet_addr("10.0.1.1"));

    
    return 0;
}
