# UPnP Device Discovery and IGD Identification

UPnP (Universal Plug and Play) utilizes a discovery protocol known as Simple Service Discovery Protocol (SSDP). This SSDP discovery service for UPnP operates as a UDP service that responds on port **1900**. To enumerate UPnP devices in your local network, you can broadcast an M-SEARCH message via the multicast address **239.255.255.250**.

When we send the M-SEARCH message during UPnP discovery, all UPnP devices, including Internet Gateway Devices (IGDs), media servers, and other networked devices in your local network, will respond. However, our primary interest is in identifying and working with IGDs. The challenge lies in the fact that there may be multiple IGDs within the local network, or in some cases, none at all, making it important to distinguish them.

In the example below, the UPnP discovery process will uncover all UPnP devices in the local network, encompassing both IGDs and other devices. However, in certain scenarios, there might be no IGDs present within your local network:

**List of all UPnP devices**

```c
#include <stdio.h>
#include "pmap.h"

void main(int argc, char *argv[]) {
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
```
**Output**

```
-----------------------------------------------------------
Host              Path          Control URL
-----------------------------------------------------------
192.168.1.1:53055	rootDesc.xml	(null)
-----------------------------------------------------------
```

As observed in the output, the "Control URL" is null. This is because the `pmap_list_upnp` function does not initiate an HTTP request to retrieve the control URL, and it cannot determine whether the corresponding endpoint is an IGD device or another type of UPnP device.

To pinpoint the specific IGD we need for external-to-internal port mapping, there are a couple of methods available. Firstly, if you are aware of the IP address of your gateway, you can filter devices based on their IP addresses. However, this approach may be less precise because a single physical device might have multiple endpoints.

A more reliable method involves identifying IGDs by their device type. IGDs typically adhere to the device type `urn:schemas-upnp-org:device:InternetGatewayDevice:1`. To determine if a device is indeed an IGD of this type, you can retrieve information about its supported device types through an HTTP request and verify that it supports the `urn:schemas-upnp-org:device:InternetGatewayDevice:1` device type. This method allows for the accurate recognition and selection of IGDs for the required port mapping tasks. To facilitate the filtering and identification of IGDs, you can use the last parameter, `PMAP_UPNP_LIST_IGD`, when calling the `pmap_list_upnp` function:

**List of all IGD devices**

```c
#include <stdio.h>
#include "pmap.h"

void main(int argc, char *argv[]) {
  pmap_url_comp_t *urls;

  printf("Request...\n");
  pmap_list_upnp(&urls, PMAP_UPNP_LIST_IGD);
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
```

**Output**

```
-----------------------------------------------------------
Host              Path          Control URL
-----------------------------------------------------------
192.168.1.1:53055	rootDesc.xml	/ctl/IPConn
-----------------------------------------------------------
```

As you can observe, the Control URL is now retrieved, making it possible to identify Internet Gateway Devices (IGDs). But keep in mind that this method  involves an HTTP request, which can result in a time penalty.

##### IGD External Port Mapping Add

If you know exactly your IGD device IP address (in our case 192.168.1.1) you can use example below to establish external-to-internal port mapping. In the given example, we assume that the Device IP address is 192.168.1.7, and the IGD gateway IP address is 192.168.1.1.

**Port mapping**

```c
#include <stdio.h>
#include "pmap.h"

void main(int argc, char *argv[]) {
  
   int ret = 0;
  char error_desc[64];
  pmap_field_t pfield;
  pfield.external_port = 6568;
  pfield.internal_port = 6568;
  pfield.lifetime_sec = 0;
  pfield.internal_ip = inet_addr("192.168.1.7");
  pfield.gateway_ip = inet_addr("192.168.1.1");
  strncpy(pfield.protocol, "TCP", sizeof(pfield.protocol));

  printf("Request...\n");
  if ((ret = pmap_upnp_addport(&pfield, error_desc, sizeof(error_desc))) == 0) {
    printf("Add port mapping to [%s => %d] lifetime=%d secs%s\n", protocol,
           port, lifetime, (lifetime == 0) ? " (no expiration)" : "");
  } else {
    printf("Error adding port mapping, error code=%d\n", ret);
  }
  
}
```

In this code snippet, a port mapping request is made, specifying the external (6568) and internal (6568) port numbers, the protocol type ("TCP" in this case), the internal IP address (192.168.1.7), and the IGD device's IP address (192.168.1.1). If the port mapping is successful, the relevant details are printed, including the port, protocol, and lifetime. If there's no expiration time set (lifetime == 0), it is indicated as having no expiration. If any error occurs during the port mapping process, the error code is displayed.

##### IGD External Port Mapping Delete

To remove an external port mapping, you can use the `pmap_upnp_delport` function. The example provided below demonstrates how this function can be used:

**Port mapping delete**

```c
#include <stdio.h>
#include "pmap.h"

void main(int argc, char *argv[]) {
  
  int ret = 0;
  char error_desc[64];
  pmap_field_t pfield;
  pfield.external_port = 6568;
  pfield.internal_port = 6568;
  pfield.gateway_ip = inet_addr("192.168.1.1");
  strncpy(pfield.protocol, "TCP", sizeof(pfield.protocol));

  printf("Request...\n");
  if ((ret = pmap_upnp_delport(&pfield, error_desc, sizeof(error_desc))) == 0) {
    printf("Delete port mapping to [%s => %d]\n", protocol, port);
  } else {
    printf("Error deleting port mapping, error code=%d\n", ret);
  }
  
}
```

In this code snippet, the purpose is to delete a port mapping. The specified external port (6568), protocol ("TCP"), internal IP address (192.168.1.7), and IGD device's IP address (192.168.1.1) are used to identify the port mapping to be deleted. If the deletion is successful, the code prints a message indicating the deletion of the specified port mapping. if any error occurs during the port mapping deletion process, the error code is displayed.

##### IGD Get External IP Address

The following code snippet demonstrates how to retrieve and display the external IP address of a specified Internet Gateway Device (IGD) using the internal IP address `192.168.1.1`

**Get external IP Address**

```c
#include <stdio.h>
#include "pmap.h"

void main(int argc, char *argv[]) {
  
  int ret = 0;
  char external_ip[16];
  char error_desc[64];
  pmap_field_t pfield;
  pfield.gateway_ip = inet_addr("192.168.1.1");
  printf("Request...\n");
  if ((ret = pmap_upnp_getexip(&pfield, external_ip, sizeof(external_ip),
                               error_desc, sizeof(error_desc))) == 0) {
    printf("External IP=[%s]\n", external_ip);
  } else {
    printf("Error getting external IP, error code=%d [%s]\n", ret, error_desc);
  }
  
}
```

**Output**

```
[10.0.1.51]
```

If the function call is successful (returns 0), the external IP address is printed within square brackets to the console. 

If you retrieve an IP address from an Internet Gateway Device (IGD) and that address is in the range of private IP address spaces, such as "192.168.*.*" or "10.0.*.*," it typically indicates the presence of another NAT (Network Address Translation) device or firewall in front of the IGD. This situation is commonly referred to as "Double NAT." This impacts of External Port Mapping more precise is involves **Limited Inbound Access** - In a Double NAT scenario, devices behind the second NAT (the IGD) may have limited or no direct inbound access from the public internet. This is because the first NAT device often doesn't have knowledge of the port mappings on the IGD. Double NAT situations is crucial, especially when you require specific network configurations, remote access to devices,  as it can affect the ability of devices in your local network to communicate with the internet and external devices. Also check [External Port Mapping Considerations](EPMC.md)

