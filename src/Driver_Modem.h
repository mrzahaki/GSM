
/* -----------------------------------------------------------------------------
 * Copyright (c) 2019-2022 Hussein zahaki. All rights reserved.
 * 
 * Based on the ARM::CMSIS driver softwares.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * $Date:        5. April 2022
 *
 * Project:      GSM
 * -------------------------------------------------------------------------- */

#ifndef MOD_DRIVER_H_
#define MOD_DRIVER_H_

#ifdef  __cplusplus
extern "C"
{
#endif

#include "modem_common.h"
// #include "MODEM_EG915U_Config.h"
#include "EG915U.h"
#include "Modem_HTTP.h"

#define MOD_API_VERSION MOD_DRIVER_VERSION_MAJOR_MINOR(0,1)  /* API version */

#define _MOD_DRIVER_(n)      MOD_DRIVER##n
#define  MOD_DRIVER_(n) _MOD_DRIVER_(n)

/****** Modem SetOption/GetOption Function Option Codes *****/
/*reserved for next releases*/
#define MOD_BSSID                      1U          ///< Station/AP Set/Get BSSID of AP to connect or of AP;        data = &bssid,    len =  6, uint8_t[6]
#define MOD_TX_POWER                   2U          ///< Station/AP Set/Get transmit power;                         data = &power,    len =  4, uint32_t: 0 .. 20 [dBm]
#define MOD_LP_TIMER                   3U          ///< Station    Set/Get low-power deep-sleep time;              data = &time,     len =  4, uint32_t [seconds]: 0 = disable (default)
#define MOD_DTIM                       4U          ///< Station/AP Set/Get DTIM interval;                          data = &dtim,     len =  4, uint32_t [beacons]
#define MOD_BEACON                     5U          ///<         AP Set/Get beacon interval;                        data = &interval, len =  4, uint32_t [ms]
#define MOD_MAC                        6U          ///< Station/AP Set/Get MAC;                                    data = &mac,      len =  6, uint8_t[6]
#define MOD_IP                         7U          ///< Station/AP Set/Get IPv4 static/assigned address;           data = &ip,       len =  4, uint8_t[4]
#define MOD_IP_SUBNET_MASK             8U          ///< Station/AP Set/Get IPv4 subnet mask;                       data = &mask,     len =  4, uint8_t[4]
#define MOD_IP_GATEWAY                 9U          ///< Station/AP Set/Get IPv4 gateway address;                   data = &ip,       len =  4, uint8_t[4]
#define MOD_IP_DNS1                    10U         ///< Station/AP Set/Get IPv4 primary   DNS address;             data = &ip,       len =  4, uint8_t[4]
#define MOD_IP_DNS2                    11U         ///< Station/AP Set/Get IPv4 secondary DNS address;             data = &ip,       len =  4, uint8_t[4]
#define MOD_IP_DHCP                    12U         ///< Station/AP Set/Get IPv4 DHCP client/server enable/disable; data = &dhcp,     len =  4, uint32_t: 0 = disable, non-zero = enable (default)
#define MOD_IP_DHCP_POOL_BEGIN         13U         ///<         AP Set/Get IPv4 DHCP pool begin address;           data = &ip,       len =  4, uint8_t[4]
#define MOD_IP_DHCP_POOL_END           14U         ///<         AP Set/Get IPv4 DHCP pool end address;             data = &ip,       len =  4, uint8_t[4]
#define MOD_IP_DHCP_LEASE_TIME         15U         ///<         AP Set/Get IPv4 DHCP lease time;                   data = &time,     len =  4, uint32_t [seconds]
#define MOD_IP6_GLOBAL                 16U         ///< Station/AP Set/Get IPv6 global address;                    data = &ip6,      len = 16, uint8_t[16]
#define MOD_IP6_LINK_LOCAL             17U         ///< Station/AP Set/Get IPv6 link local address;                data = &ip6,      len = 16, uint8_t[16]
#define MOD_IP6_SUBNET_PREFIX_LEN      18U         ///< Station/AP Set/Get IPv6 subnet prefix length;              data = &len,      len =  4, uint32_t: 1 .. 127
#define MOD_IP6_GATEWAY                19U         ///< Station/AP Set/Get IPv6 gateway address;                   data = &ip6,      len = 16, uint8_t[16]
#define MOD_IP6_DNS1                   20U         ///< Station/AP Set/Get IPv6 primary   DNS address;             data = &ip6,      len = 16, uint8_t[16]
#define MOD_IP6_DNS2                   21U         ///< Station/AP Set/Get IPv6 secondary DNS address;             data = &ip6,      len = 16, uint8_t[16]
#define MOD_IP6_DHCP_MODE              22U         ///< Station/AP Set/Get IPv6 DHCPv6 client mode;                data = &mode,     len =  4, uint32_t: MOD_IP6_DHCP_xxx (default Off)

/****** Modem Security Type *****/
/*reserved for the next releases*/
#define MOD_SECURITY_OPEN              0U          ///< Open
#define MOD_SECURITY_WEP               1U          ///< Wired Equivalent Privacy (WEP) with Pre-Sheared Key (PSK)
#define MOD_SECURITY_WPA               2U          ///< Modem Protected Access (WPA) with PSK
#define MOD_SECURITY_WPA2              3U          ///< Modem Protected Access II (WPA2) with PSK
#define MOD_SECURITY_UNKNOWN           255U        ///< Unknown

/****** Modem Protected Setup (WPS) Method *****/
/*reserved for the next releases*/
#define MOD_WPS_METHOD_NONE            0U          ///< Not used
#define MOD_WPS_METHOD_PBC             1U          ///< Push Button Configuration
#define MOD_WPS_METHOD_PIN             2U          ///< PIN

/****** Modem IPv6 Dynamic Host Configuration Protocol (DHCP) Mode *****/
/*reserved for the next releases*/
#define MOD_IP6_DHCP_OFF               0U          ///< Static Host Configuration (default)
#define MOD_IP6_DHCP_STATELESS         1U          ///< Dynamic Host Configuration stateless DHCPv6
#define MOD_IP6_DHCP_STATEFULL         2U          ///< Dynamic Host Configuration statefull DHCPv6

/****** Modem Event *****/
/*reserved for the next releases*/
#define MOD_EVENT_CONNECT          (1UL << 0)   ///< : Station has connected;           
#define MOD_EVENT_DISCONNECT       (1UL << 1)   ///< : Station has disconnected;        
#define MOD_EVENT_ETH_RX_FRAME        (1UL << 4)   /// reserved
#define MOD_EVENT_READY                (1UL << 5)


/**
\brief Modem Configuration
*/
typedef struct MOD_CONFIG_s {
  const char   *ssid;                                   ///< Pointer to Service Set Identifier (SSID) null-terminated string
  const char   *pass;                                   ///< Pointer to Password null-terminated string
        uint8_t security;                               ///< Security type (MOD_SECURITY_xxx)
        uint8_t ch;                                     ///< Modem Channel (0 = auto, otherwise = exact channel)
        uint8_t reserved;                               ///< Reserved
        uint8_t wps_method;                             ///< Modem Protected Setup (WPS) method (MOD_WPS_METHOD_xxx)
  const char   *wps_pin;                                ///< Pointer to Modem Protected Setup (WPS) PIN null-terminated string
} MOD_CONFIG_t;

/**
\brief Modem Scan Information
*/
typedef struct MOD_SCAN_INFO_s {
  char    ssid[32+1];                                   ///< Service Set Identifier (SSID) null-terminated string
  uint8_t bssid[6];                                     ///< Basic Service Set Identifier (BSSID)
  uint8_t security;                                     ///< Security type (MOD_SECURITY_xxx)
  uint8_t ch;                                           ///< Modem Channel
  uint8_t rssi;                                         ///< Received Signal Strength Indicator
} MOD_SCAN_INFO_t;

/**
\brief Modem Network Information
*/
typedef struct MOD_NET_INFO_s {
  char    ssid[32+1];                                   ///< Service Set Identifier (SSID) null-terminated string
  char    pass[64+1];                                   ///< Password null-terminated string
  uint8_t security;                                     ///< Security type (MOD_SECURITY_xxx)
  uint8_t ch;                                           ///< Modem Channel
  uint8_t rssi;                                         ///< Received Signal Strength Indicator
} MOD_NET_INFO_t;


typedef struct {
    HTTP_HeaderField * fields;
    uint32_t nfield;
    char * buffer; //static allocation must be with size of > MOD_HeaderField + url + (nfield + 1) * 2 + 60
    
} MOD_Header_t;

typedef void (*HTTP_ResponseCallback_t) (uint8_t * data, uint32_t counter, uint32_t remain_size, uint32_t size);
typedef uint32_t (*HTTP_RequestCallback_t) (uint8_t ** data, uint32_t counter, uint32_t remain_size);

typedef struct {
  unsigned method: 1;
  uint8_t *url; // server URl
  HTTP_RequestCallback_t data_callback;// optional, incase of NULL data, must return new data size
  uint8_t *data; //TX data buffer
  uint32_t data_length;  

  MOD_Header_t *header; //request header optional
  uint32_t url_length; 

  uint8_t *response; // RX data buffer optional
  HTTP_ResponseCallback_t response_callback;
  uint32_t response_length; //Optional (if the response buffer is not null, the required response length must be taken)

  unsigned response_header: 1;
  uint32_t timeout;

  uint32_t resptime;

} MOD_HTTP_t;
 
#define MOD_HTTP_GET          0
#define MOD_HTTP_POST         1
#define MOD_HTTP_NORESP       NULL
#define MOD_HTTP_AUTO_LENGTH  NULL
#define MOD_HTTP_ENABLE       1

/****** Socket Address Family definitions *****/
#define ARM_SOCKET_AF_INET                  1           ///< IPv4
#define ARM_SOCKET_AF_INET6                 2           ///< IPv6

/****** Socket Type definitions *****/
#define ARM_SOCKET_SOCK_STREAM              1           ///< Stream socket
#define ARM_SOCKET_SOCK_DGRAM               2           ///< Datagram socket

/****** Socket Protocol definitions *****/
#define ARM_SOCKET_IPPROTO_TCP              1           ///< TCP
#define ARM_SOCKET_IPPROTO_UDP              2           ///< UDP

/****** Socket Option definitions *****/
#define ARM_SOCKET_IO_FIONBIO               1           ///< Non-blocking I/O (Set only, default = 0); opt_val = &nbio, opt_len = sizeof(nbio), nbio (integer): 0=blocking, non-blocking otherwise
#define ARM_SOCKET_SO_RCVTIMEO              2           ///< Receive timeout in ms (default = 0); opt_val = &timeout, opt_len = sizeof(timeout)
#define ARM_SOCKET_SO_SNDTIMEO              3           ///< Send timeout in ms (default = 0); opt_val = &timeout, opt_len = sizeof(timeout)
#define ARM_SOCKET_SO_KEEPALIVE             4           ///< Keep-alive messages (default = 0); opt_val = &keepalive, opt_len = sizeof(keepalive), keepalive (integer): 0=disabled, enabled otherwise
#define ARM_SOCKET_SO_TYPE                  5           ///< Socket Type (Get only); opt_val = &socket_type, opt_len = sizeof(socket_type), socket_type (integer): ARM_SOCKET_SOCK_xxx

/****** Socket Function return codes *****/
#define ARM_SOCKET_ERROR                   (-1)         ///< Unspecified error
#define ARM_SOCKET_ESOCK                   (-2)         ///< Invalid socket
#define ARM_SOCKET_EINVAL                  (-3)         ///< Invalid argument
#define ARM_SOCKET_ENOTSUP                 (-4)         ///< Operation not supported
#define ARM_SOCKET_ENOMEM                  (-5)         ///< Not enough memory
#define ARM_SOCKET_EAGAIN                  (-6)         ///< Operation would block or timed out
#define ARM_SOCKET_EINPROGRESS             (-7)         ///< Operation in progress
#define ARM_SOCKET_ETIMEDOUT               (-8)         ///< Operation timed out
#define ARM_SOCKET_EISCONN                 (-9)         ///< Socket is connected
#define ARM_SOCKET_ENOTCONN                (-10)        ///< Socket is not connected
#define ARM_SOCKET_ECONNREFUSED            (-11)        ///< Connection rejected by the peer
#define ARM_SOCKET_ECONNRESET              (-12)        ///< Connection reset by the peer
#define ARM_SOCKET_ECONNABORTED            (-13)        ///< Connection aborted locally
#define ARM_SOCKET_EALREADY                (-14)        ///< Connection already in progress
#define ARM_SOCKET_EADDRINUSE              (-15)        ///< Address in use
#define ARM_SOCKET_EHOSTNOTFOUND           (-16)        ///< Host not found

#define MOD_DEFAULT_CONTEXT   MOD_CONTEXT_CONFIG_DEFAULT
#define MOD_CONTEXT_1               1
#define MOD_CONTEXT_2               2
#define MOD_CONTEXT_3               3
#define MOD_CONTEXT_4               4 
#define MOD_CONTEXT_5               5
#define MOD_CONTEXT_6               6
#define MOD_CONTEXT_7               7   
#define MOD_ACTIVATE_DEFAULT        NULL
#define HTTP_SETOPTION_ENABLE       1
#define HTTP_SETOPTION_DISABLE      0
#define HTTP_SETOPTION_CONTENT_TYPE_X_WWW_FORM_URLENCODED   0
#define HTTP_SETOPTION_CONTENT_TYPE_TEXT                    1
#define HTTP_SETOPTION_CONTENT_TYPE_OCTET                   2
#define HTTP_SETOPTION_CONTENT_TYPE_FORM_DATA               2
#define HTTP_SETOPTION_RESET                                NULL


typedef void (*MOD_SignalEvent_t) (uint32_t event, void *arg); ///< Pointer to \ref MOD_SignalEvent : Signal Modem Event.


/**
\brief Modem Driver Capabilities.
*/
typedef struct _MOD_CAPABILITIES {
  uint32_t event_ap_connect      : 1;   ///< : event generated on connect
  uint32_t event_ap_disconnect   : 1;   ///< : event generated on disconnect
  uint32_t event_eth_rx_frame    : 1;   ///< Event generated on Ethernet frame reception in bypass mode
  uint32_t bypass_mode           : 1;   ///< Bypass or pass-through mode (Ethernet interface)
  uint32_t ip                    : 1;   ///< IP (UDP/TCP) (Socket interface)
  uint32_t ip6                   : 1;   ///< IPv6 (Socket interface)
  uint32_t ping                  : 1;   ///< Ping (ICMP)
  uint32_t reserved              : 20;  ///< Reserved (must be zero)
} MOD_CAPABILITIES;
 
/**
\brief Access structure of the Modem Driver.
*/
typedef struct _MOD_DRIVER {
  MOD_DRIVER_VERSION (*GetVersion)          (void);
  MOD_CAPABILITIES   (*GetCapabilities)     (void);
  int32_t            (*Initialize)          (MOD_SignalEvent_t cb_event);
  int32_t            (*Uninitialize)        (void);
  int32_t            (*PowerControl)        (MOD_POWER_STATE state);
  int32_t            (*Activate)            (MOD_CONTEXT_CONFIG * context, MOD_PDP_CONTEXT * pdp);
  int32_t            (*Deactivate )         (int32_t socket);
  int32_t            (*Context)             (MOD_CONTEXT_CONFIG * context);
  int32_t            (*HTTP_SetOption)      (MOD_HTTPOption_t option, void *data);
  int32_t            (*HTTP_GetOption)      (MOD_HTTPOption_t option, char *resp, uint16_t length)  ;
  int32_t            (* HTTP_SetURL)        (char * url, uint32_t length, uint32_t timeout);
  int32_t            (*HTTP)               (int32_t socket, MOD_HTTP_t * httpd);
  int32_t            (*release)            (void);
  int32_t            (*SSL_SetOption)      (SSL_Config_t option, uint8_t ssl_context_id, void * data);

} const MOD_DRIVER;

#ifdef  __cplusplus
}
#endif

#endif /* MOD_DRIVER_H_ */
