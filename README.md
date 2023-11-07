# Port Mapping Library - libpmap

The library to manage Internet Gateway external port mapping with **UPnP-IGDP** and **NAT-PMP** written in C. Additionally, it includes a Command Line Interface tool that can be used for testing and experimentation

The Internet Gateway External Port Mapping Library is a standalone C library designed to manage external port mappings on Internet Gateway Devices (IGDs) using both UPnP (Universal Plug and Play) and NAT-PMP (Network Address Translation Port Mapping Protocol). The library is built with a focus on **minimal dependencies**, relying solely on **POSIX** standard C functions and **BSD**-like socket operations. This makes it easy to integrate into various C-based applications without introducing complex dependencies.

NAT-PMP and UPnP-IGDP are protocols that help with configuring port forwarding in NAT routers, making it easier for devices and services on a local network to be accessible from the internet. NAT-PMP is a simpler and more secure option often used by Apple devices, while UPnP-IGDP is a more comprehensive protocol suite used by a wider range of devices but has some security concerns. 

UPnP-IGDP and NAT-PMP port forwarding services are usually activated by default on the majority of consumer-grade internet gateway NAT routers. This default setting allows devices within the internal network to autonomously configure the required TCP and UDP port forwarding operations on the outward-facing router. This empowers external devices to establish connections with services hosted on your internal network without the need for manual configuration.

This protocols should only be used when the client determines that its primary IPv4 address is in one of the private IPv4 address ranges defined in "Address Allocation for Private Internets" [RFC1918].   This includes the address ranges `10/8`, `172.16/12`, and `192.168/16`.

***NOTE: The project is under active development and anything can change at any time.***



## Build

The program tested on Mac OS X and Linux machine. It may build in Windows platforms (we did not test).    

Now you can build `pmap`

    > cd libpmap
    > make clean && make

This is **not mandatory** but you can make distribution binary file by typing in terminal window:

    > make dist

First type without arguments from command line:

`./pmap`

In terminal output window you will see options:

    usage: ./pmap < -a | -d | -e > < -u | -p > <args>
      -a    Add port mapping
            <args>: <external port> <my_IPv4> <gateway_IPv4> <protocol> <lifetime>
      -d    Delete port mapping
            <args>: <external port> <gateway_IPv4> <protocol>
      -e    Get external IP address
            <args>: <gateway_IPv4>
      -l    Print list of available IGDs (UPnP)
      -p    Using NAT-PMP protocol for port mapping
      -u    Using UPnP protocol for port mapping
      -v    show request => response debug output
      -h    show this help and exit
    Example 1: ./pmap -l
    Example 2: ./pmap -u -a 6568 192.168.1.7 192.168.1.1 TCP 7200
    Example 3: ./pmap -u -d 6568 192.168.1.1 TCP
    Example 4: ./pmap -u -e 192.168.1.1



## How to use `pmap` CLI

#### Print list of available gateways (UPnP-IGDP)

The provided command is used to list available gateways in the local network that support UPnP-IGDP protocols for managing port mappings. This command is specific to UPnP-IGDP because NAT-PMP is only available on the default gateway IP address to which you are currently connected.

```
./pmap -l
```

Possible output:

```
Request...
-----------------------------------------------------------
Host			Path		Control URL
-----------------------------------------------------------
192.168.1.1:53055	rootDesc.xml	/ctl/IPConn
-----------------------------------------------------------
```

#### Add port mapping

The provided command is used to create a UPnP-IGDP port mapping, which forwards external traffic from port `6569` to an internal device at IP address `192.168.1.7` on port `6569`, using the `TCP` protocol, with a lifetime of `7200` seconds. You can choose to use NAT-PMP instead by changing the first argument of the command line: `-u` indicates UPnP-IGDP, and `-p` indicates NAT-PMP. Therefore, you can replace `-u` with `-p` to use NAT-PMP.

```
./pmap -u -a 6569 192.168.1.7 192.168.1.1 TCP 7200
```

The following options and arguments:

- `-u`  Indicates that you want to use UPnP-IGDP for port mapping (you can use `-p` to use NAT-PMP).
- `-a` Specifies that you want to add a port mapping.
- `6569`  This is the external port, the port number on the router's public IP address that you want to map to an internal device.
- `192.168.1.7`  This is the internal IP address of the device within your local network that you want to forward traffic to.
- `192.168.1.1`  This is the IP address of your gateway or router.
- `TCP`  This indicates the protocol you want to use for the port mapping, in this case, TCP.
- `7200`  This is the lifetime of the port mapping in seconds, which specifies how long the port mapping should remain active. 0 indicates no expiration.

Possible Output:

```
Request...
Add port mapping to [TCP => 6569] lifetime=7200 secs
```

After executing the command. Here's a breakdown of the output:

"Add port mapping to [TCP => 6569] lifetime=7200 secs": This part of the output signifies the successful addition of the port mapping.

#### Delete port mapping

The provided command is used to delete a UPnP-IGDP port mapping associated with the specified external port number `6569` and protocol `TCP` on the router or gateway with the IP address `192.168.1.1`. The output confirms the successful removal of the port mapping. To achieve this using NAT-PMP, you can change the first argument of the command line. Use `-u` for UPnP-IGDP and `-p` for NAT-PMP

```
./pmap -u -d 6569 192.168.1.1 TCP
```

The following options and arguments:

- `-u` Indicates that you want to use UPnP-IGDP for port mapping (you can use `-p` to use NAT-PMP).
- `-d` Specifies that you want to delete a port mapping.
- `6569` This is the external port number that you want to delete the mapping for.
- `192.168.1.1` This is the IP address of the router or gateway from which you want to remove the port mapping.
- `TCP` This indicates the protocol of the port mapping to be deleted, which is TCP in this case.

Possible output:

```
Request...
Delete port mapping to [TCP => 6569]
```

"Delete port mapping to [TCP => 6569]": This part of the output signifies the successful deletion of the port mapping.

#### Get external IP

The provided command is used to query a router or gateway with the IP address `192.168.1.1` and retrieve its external IP address using UPnP. This information can be helpful in various networking scenarios where knowing the external IP address is necessary, such as for remote access or dynamic DNS services. To achieve the same with NAT-PMP, simply change the first argument of the command line. Use `-u` for UPnP or `-p` for NAT-PMP:

```
./pmap -u -e 192.168.1.1
```

The following options and arguments:

- `-u` Indicates that you want to use UPnP-IGDP (you can use `-p` to use NAT-PMP).
- `-e` Specifies that you want to retrieve the external IP address.
- `192.168.1.1` This is the IP address of the router or gateway from which you want to retrieve the external IP address.

Possible output:

```
Request...
External IP=[10.0.1.51]
```

"External IP=[10.0.1.51]": This part of the output provides the external IP address that has been obtained. In this case, the external IP address is "10.0.1.51."



## How to use library

- [NAT-PMP and UPnP-IGDP](docs/NPMP_UPnP.md)
- [External Port Mapping Considerations](docs/EPMC.md)



## Private IPv4 Addresses

| RFC 1918 name | IP address range              | Mask bits |
| :------------ | :---------------------------- | :-------- |
| 24-bit block  | 10.0.0.0 – 10.255.255.255     | 8 bits    |
| 20-bit block  | 172.16.0.0 – 172.31.255.255   | 12 bits   |
| 16-bit block  | 192.168.0.0 – 192.168.255.255 | 16 bits   |



## Resources

https://www.rapid7.com/blog/post/2020/12/22/upnp-with-a-holiday-cheer/

- [IGDP](http://en.wikipedia.org/wiki/Internet_Gateway_Device_Protocol)
  - https://openconnectivity.org/developer/specifications/upnp-resources/upnp/internet-gateway-device-igd-v-2-0/

- [STUN](http://en.wikipedia.org/wiki/Session_Traversal_Utilities_for_NAT)
- [ICE](http://en.wikipedia.org/wiki/Interactive_Connectivity_Establishment)
- [NAT-PMP](http://en.wikipedia.org/wiki/NAT_Port_Mapping_Protocol)
  - https://www.rfc-editor.org/rfc/rfc6886
- [PCP](http://en.wikipedia.org/wiki/Port_Control_Protocol)
  - https://www.rfc-editor.org/rfc/rfc6887
- [UPnP](http://en.wikipedia.org/wiki/Universal_Plug_and_Play)
  - https://www.rfc-editor.org/rfc/rfc6970
  - https://openconnectivity.org/upnp-specs/UPnP-arch-DeviceArchitecture-v2.0-20200417.pdf

