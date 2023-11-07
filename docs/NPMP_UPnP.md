# Using libpmap Library

NAT-PMP and UPnP-IGD are two distinct protocols used to simplify network configuration and enable devices to interact seamlessly within a local network. While UPnP relies on the Simple Service Discovery Protocol (SSDP) for device discovery, NAT-PMP follows a different approach to facilitate port mapping for networked devices.

NAT-PMP, unlike UPnP, is specifically designed for managing port mappings in network address translation (NAT) routers. It operates as a lightweight and efficient protocol to establish communication between devices within a local network and the external internet. The primary goal of NAT-PMP is to make it easier for devices, such as gaming consoles, remote desktop applications, or peer-to-peer file sharing programs, to establish connections through the NAT router to the wider internet.

UPnP (Universal Plug and Play) utilizes a discovery protocol known as Simple Service Discovery Protocol (SSDP). This SSDP discovery service for UPnP operates as a UDP service that responds on port **1900**. To enumerate UPnP devices in your local network, you can broadcast an M-SEARCH message via the multicast address **239.255.255.250**. When we send the M-SEARCH message during UPnP discovery, all UPnP devices, including Internet Gateway Devices (IGDs), media servers, and other networked devices in your local network, will respond. However, our primary interest is in identifying and working with IGDs. The challenge is in the fact that there may be multiple IGDs within the local network, or in some cases, none at all, making it important to distinguish them.

In the case of NAT-PMP, there's no use of SSDP and multicast addresses. Instead, devices within the local network can directly communicate with the NAT router using  (UDP) for its communication, and it operates on a specific port number 5351 to request the opening of specific ports for incoming traffic. One of the outstanding features of NAT-PMP is its ability to enable a client to request a specific external port. If that external port is already in use by another client, the NAT-PMP server will intelligently allocate an alternative available external port. This is a crucial feature because in the case of UPnP, if an external port is unavailable for any reason, you would need to guess which port is available for managing your own port mapping. Furthermore, this can lead to numerous request-response interactions with the IGD, making successful mapping uncertain. Check RFC 6886 (9.4.  Atomic Allocation Operations)

## Library

All functions are blocking and work in synchronous mode. Therefore, the waiting time depends on the size of the internal network. Functions such as `pmap_upnp_addport`, `pmap_upnp_delport`, and `pmap_upnp_getexip` take approximately 2-4 seconds to complete their tasks. If you're wondering why it takes this long, you can attribute it to the UPnP protocol. On the other hand, the same functions in the NAT-PMP protocol take around 250ms. More details are discussed in the [Performance](#performance) section. 

All functions follow a simple convention: they return 0 for success and 1 for an error. To pinpoint the cause of an error, it's advisable to check the `errno` value. Additionally, available error descriptions provided by the function for a more detailed understanding of the issue.

In the example below, the UPnP discovery process will uncover all UPnP devices in the local network, encompassing both IGDs and other devices. However, there might be no IGDs present within your local network. This example is specific to UPnP-IGDP because NAT-PMP is only available on the default gateway IP address to which you are currently connected and there's no need to list them separately.

**List of all UPnP devices**

```c
#include <stdio.h>
#include "pmap_npmp.h"
#include "pmap_upnp.h"

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

#### **List of all IGD devices**

```c
#include <stdio.h>
#include "pmap_npmp.h"
#include "pmap_upnp.h"

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

As you can observe, the Control URL is now retrieved, making it possible to identify IGDs. But keep in mind that this method involves an HTTP request, which can result in a time penalty.

#### Add port mapping

If you know exactly your IGD device IP address (in our case 192.168.1.1) you can use example below to establish external-to-internal port mapping. In the given example, we assume that the Device IP address is 192.168.1.7, and the IGD gateway IP address is 192.168.1.1.

**Example IGD**

```c
#include <stdio.h>
#include "pmap_npmp.h"
#include "pmap_upnp.h"

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
    printf("Error adding port mapping, error code=%d [%s]\n", errno,
           error_desc);
  }
  
}
```

For NAT-PMP, you do not need to specify the internal IP address; it is sufficient to specify the gateway IP address. In the provided example, we make the assumption that the NAT-PMP gateway IP address is 192.168.1.1.

**Example NAT-PMP**

```c
#include <stdio.h>
#include "pmap_npmp.h"
#include "pmap_upnp.h"

void main(int argc, char *argv[]) {
  
  int ret = 0;
  char error_desc[64];
  pmap_field_t pfield;
  pfield.external_port = 6568;
  pfield.internal_port = 6568;
  pfield.lifetime_sec = 0;
  pfield.gateway_ip = inet_addr("192.168.1.1");
  strncpy(pfield.protocol, "TCP", sizeof(pfield.protocol));

  printf("Request...\n");
  if ((ret = pmap_npmp_addport(&pfield, error_desc, sizeof(error_desc))) == 0) {
    printf("Add port mapping to [%s => %d] lifetime=%d secs%s\n", protocol,
           pfield.external_port, pfield.lifetime_sec,
           (lifetime == 0) ? " (no expiration)" : "");
  } else {
    printf("Error adding port mapping, error code=%d [%s]\n", errno,
           error_desc);
  }
  
}
```

In this code snippet(s), a port mapping request is made, specifying the external (6568) and internal (6568) port numbers, the protocol type ("TCP" in this case), the internal IP address (192.168.1.7 in case of IGD), and the IGD/NAT-PMP device's IP address (192.168.1.1). If the port mapping is successful, the relevant details are printed, including the port, protocol, and lifetime. If there's no expiration time set (lifetime == 0), it is indicated as having no expiration. If any error occurs during the port mapping process, the error code is displayed. It's mandatory to verify the value of `pfield.external_port` returned by `pmap_npmp_addport` function. This is because NAT-PMP may alter the external port, especially when the requested port is already in use. However, this behavior is unique to NAT-PMP and does not apply to UPnP-IGD, as IGDs operating with UPnP typically do not have this port modification capability.

#### Delete port mapping

To remove an external port mapping, you can use the `pmap_upnp_delport` function. The example provided below demonstrates how this function can be used:

**Example IGD**

```c
#include <stdio.h>
#include "pmap_npmp.h"
#include "pmap_upnp.h"

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
    printf("Error deleting port mapping, error code=%d [%s]\n", errno,
           error_desc);
  }
  
}
```

**Example NAT-PMP**

```c
#include <stdio.h>
#include "pmap_npmp.h"
#include "pmap_upnp.h"

void main(int argc, char *argv[]) {
  
  int ret = 0;
  char error_desc[64];
  pmap_field_t pfield;
  pfield.external_port = 6568;
  pfield.internal_port = 6568;
  pfield.gateway_ip = inet_addr("192.168.1.1");
  strncpy(pfield.protocol, "TCP", sizeof(pfield.protocol));

  printf("Request...\n");
  if ((ret = pmap_npmp_delport(&pfield, error_desc, sizeof(error_desc))) == 0) {
    printf("Delete port mapping to [%s => %d]\n", protocol, port);
  } else {
    printf("Error deleting port mapping, error code=%d [%s]\n", errno,
           error_desc);
  }
  
}
```

In this code snippet, the purpose is to delete a port mapping. The specified external port (6568), protocol ("TCP") and NAT-PMP/IGD device's IP address (192.168.1.1) are used to identify the port mapping to be deleted. If the deletion is successful, the code prints a message indicating the deletion of the specified port mapping. if any error occurs during the port mapping deletion process, the error code is displayed.

#### Get External IP Address

The following code snippet demonstrates how to retrieve and display the external IP address using the NAT-PMP or IGD gateway IP address `192.168.1.1`

**Example IGD**

```c
#include <stdio.h>
#include "pmap_npmp.h"
#include "pmap_upnp.h"

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

**Example NAT-PMP**

```c
#include <stdio.h>
#include "pmap_npmp.h"
#include "pmap_upnp.h"

void main(int argc, char *argv[]) {
  
  int ret = 0;
  char external_ip[16];
  char error_desc[64];
  pmap_field_t pfield;
  pfield.gateway_ip = inet_addr("192.168.1.1");
  
  printf("Request...\n");
  if ((ret = pmap_npmp_getexip(&pfield, external_ip, sizeof(external_ip),
                               error_desc, sizeof(error_desc))) == 0) {
    printf("External IP=[%s]\n", external_ip);
  } else {
    printf("Error getting external IP, error code=%d [%s]\n", errno,
           error_desc);
  }
}
```

**Output**

```
[10.0.1.51]
```

If the function call is successful (returns 0), the external IP address is printed within square brackets to the console. 

If you retrieve an IP address from an gateway and that address is in the range of private IP address spaces, such as "192.168.*.*" or "10.0.*.*," it typically indicates the presence of another NAT (Network Address Translation) device or firewall in front of the NAT-PMP/IGD. This situation is commonly referred to as "Double NAT." This impacts of External Port Mapping more precise is involves **Limited Inbound Access** - In a Double NAT scenario, devices behind the second NAT may have limited or no direct inbound access from the public internet. This is because the first NAT device often doesn't have knowledge of the port mappings on the NAT-PMP/IGD. Double NAT situations is crucial, especially when you require specific network configurations, remote access to devices,  as it can affect the ability of devices in your local network to communicate with the internet and external devices. Also check [External Port Mapping Considerations](EPMC.md)



## Performance

UPnP IGD port mapping performance is very weak. It retrieves the external IPv4 address with greater complexity. The XML responses used for this purpose are unbounded in size, often ranging from 4,000 to 8,000 bytes. The protocol does not impose an upper limit on the response size, potentially resulting in even larger data transfers. Port mapping in UPnP IGD is typically accomplished through a 14-packet exchange, involving TCP connection setup, data exchange, and connection teardown.

For example, the process of discovering the external IPv4 address of a UPnP IGD home gateway involves the following steps:

1. **SSDP Transaction:** This step is used to determine the TCP port to use and the "URL" of the XML document to fetch from the gateway. Following the SSDP specification, it requires 3 multicast requests, which generate 9 unicast responses.
2. **HTTP 'GET' Request for Device Description:** Typically, this entails 16 packets, including 3 packets for TCP connection setup, 9 packets for data exchange, and a 4-packet FIN-ACK-FIN-ACK sequence to close the connection.
3. **HTTP 'POST' Request for External IPv4 Address:** This step typically involves 14 packets, including 3 packets for TCP connection setup, 7 packets for data exchange, and a 4-packet FIN-ACK-FIN-ACK sequence to close the connection.

This is unfortunate because these steps represent the series of actions necessary to obtain a single external IPv4 address.

On the other hand, when it comes to fetching the external IPv4 address, NAT-PMP accomplishes this with a concise 2-packet UDP exchange, involving a 44-byte request and a 54-byte response. In contrast, UPnP IGD requires a much more expensive process, utilizing 42 packets and consuming thousands of bytes for the same operation.



## Troubleshooting

### Debug Log Trace

There are two types of debug output mechanisms in library:

- Compile-Time Debug Output:

  - During the compile time, you can choose to enable or disable certain debugging features. If you want to turn off compile-time debugging, you would need to recompile the project without the debug output macros. This means modifying build configuration to exclude debug-related code.
- Runtime Debug Output:

  - This type of debug output can be enabled or disabled while the program is running. When you activate runtime debug mode, it allows you to control the visibility of network packet requests and responses, including UDP and HTTP traffic.
  - You can toggle the runtime debug mode on and off without the need to recompile the entire program. This is especially useful for monitoring and diagnosing UPnP and NAT-PMP issues during the execution of the program.

To enable compile-time debugging, you should set the `PMAP_DEBUG_LOG_DEBUG` and `PMAP_DEBUG_LOG_ERROR` value to `1` in the `pmap_cfg.h` file, and then recompile the project to apply the changes to the final binary.

To enable runtime debug output, you can call the function `pmap_set_debug(true)` to turn on debug output or `false` to turn it off during program runtime.

### Packet Timeout

Because, UPnP and NAT-PMP protocols based on UDP layer (IGD also uses TCP) the network packets can sometimes be lost or delayed, and the response from the router may not arrive in time. To mitigate this issue, the software or library allows you to adjust the timeout value for waiting for a response from the router. The default timeout is set to 2 seconds, both for UDP and TCP calls. If you find that the first call to `pmap_list_upnp` often results in an empty list due to packet loss or delays, you can increase the timeout value by changing the `PMAP_DEFAULT_WAIT_TIMEOUT` value in the `pmap_cfg.h` file. By increasing the timeout, you provide the router with more time to respond, reducing the likelihood of an empty list in the function's response. 

As mentioned earlier, the initial call to `pmap_list_upnp` may occasionally yield an empty list of results. In such situations, it is recommended to make a subsequent attempt to confirm that the request has not been lost.
