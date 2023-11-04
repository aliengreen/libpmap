/*
 *    upnp_msg.h
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

#ifndef UPNP_MSG_H
#define UPNP_MSG_H

/* M-SEARCH body */
const static char *m_search = "M-SEARCH * HTTP/1.1\r\n"
                              "HOST: 239.255.255.250:1900\r\n"
                              "MAN: \"ssdp:discover\"\r\n"
                              "MX: 5\r\n"
                              "ST: upnp:rootdevice\r\n"
                              "\r\n";

/**
 * SOAP request body for adding a port mapping in the context of UPnP. It
 * includes placeholders for various parameters such as external port, protocol,
 * internal port, internal client IP, enabled status, port mapping description,
 * and lease duration. Later we can fill in these placeholders with specific
 * values to create a SOAP HTTP POST request.
 */
const static char *soap_action_add =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
    "<s:Envelope "
    "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
    "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
    "  <s:Body>\r\n"
    "    <u:AddPortMapping "
    "      xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">\r\n"
    "      <NewRemoteHost></NewRemoteHost>\r\n"
    "      <NewExternalPort>%d</NewExternalPort>\r\n"
    "      <NewProtocol>%s</NewProtocol>\r\n"
    "      <NewInternalPort>%d</NewInternalPort>\r\n"
    "      <NewInternalClient>%s</NewInternalClient>\r\n"
    "      <NewEnabled>True</NewEnabled>\r\n"
    "      <NewPortMappingDescription>pMAP</NewPortMappingDescription>\r\n"
    "      <NewLeaseDuration>%d</NewLeaseDuration>\r\n"
    "    </u:AddPortMapping>\r\n"
    "  </s:Body>\r\n"
    "</s:Envelope>\r\n";

/**
 * SOAP request body for delete a port mapping in the context of UPnP. It
 * includes placeholders for  external port and protocol. Later we can fill in
 * these placeholders with specific values to create a SOAP HTTP POST request.
 */
const static char *soap_action_del =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
    "<s:Envelope "
    "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
    "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
    "  <s:Body>\r\n"
    "    <u:DeletePortMapping "
    "      xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">\r\n"
    "      <NewRemoteHost></NewRemoteHost>\r\n"
    "      <NewExternalPort>%d</NewExternalPort>\r\n"
    "      <NewProtocol>%s</NewProtocol>\r\n"
    "    </u:DeletePortMapping>\r\n"
    "  </s:Body>\r\n"
    "</s:Envelope>\r\n";

/**
 * SOAP request body for delete a port mapping in the context of UPnP. It
 * includes placeholders for  external port and protocol. Later we can fill in
 * these placeholders with specific values to create a SOAP HTTP POST request.
 */
const static char *soap_action_getextip =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
    "<s:Envelope "
    "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
    "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
    "  <s:Body>\r\n"
    "    <u:GetExternalIPAddress "
    "      xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">\r\n"
    "    </u:GetExternalIPAddress>\r\n"
    "  </s:Body>\r\n"
    "</s:Envelope>\r\n";

#endif // UPNP_MSG_H
