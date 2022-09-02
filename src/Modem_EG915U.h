
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
#ifndef MOD_EG915U_H__
#define MOD_EG915U_H__

#include <string.h>

#include "DRIVER_MODEM.h"                // ::CMSIS Driver:Modem
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2

#include "EG915U.h"

#define MOD_SERIAL_BAUDRATE        MOD_EG915U_SERIAL_BAUDRATE
#define MOD_DRIVER_NUMBER          MOD_EG915U_DRIVER_NUMBER

/* Command response timeout [ms] (default) */
#ifndef MOD_RESP_TIMEOUT
#define MOD_RESP_TIMEOUT           (5000)
#endif
 
/* Connection open timeout [ms] (default) */
#ifndef MOD_CONNOPEN_TIMEOUT
#define MOD_CONNOPEN_TIMEOUT       (20000)
#endif
 
/* Socket accept timeout */
#ifndef MOD_SOCKET_ACCEPT_TIMEOUT
#define MOD_SOCKET_ACCEPT_TIMEOUT  (0)
#endif

/* Modem thread pooling interval [ms] */
#ifndef MOD_THREAD_POOLING_TIMEOUT
#define MOD_THREAD_POOLING_TIMEOUT (20)
#endif 

/*  default channel (used when channel not specified in Activate) */
#ifndef MOD_AP_CHANNEL
#define MOD_AP_CHANNEL             (2)
#endif

/* AT response echo enable/disable */
#ifndef MOD_AT_ECHO
#define MOD_AT_ECHO                (1)
#endif

/* MODEM interface definitions */
#define MOD_INTERFACE_STATION      0
#define MOD_INTERFACE_AP           1

/* MODEM thread flags */
#define MOD_THREAD_RX_DATA         0x01    // Data received
#define MOD_THREAD_RX_ERROR        0x02    // Data receive error
#define MOD_THREAD_TX_DONE         0x04    // Data transmitted
#define MOD_THREAD_TERMINATE       0x08    // Terminate thread
#define MOD_THREAD_KICK            0x10    // Wake-up

#define MOD_THREAD_FLAGS          (MOD_THREAD_RX_DATA  | \
                                    MOD_THREAD_RX_ERROR | \
                                    MOD_THREAD_TX_DONE  | \
                                    MOD_THREAD_TERMINATE| \
                                    MOD_THREAD_KICK)

/* MODEM wait flags */
#define MOD_WAIT_RESP_GENERIC      (1U <<   0)
#define MOD_WAIT_TX_REQUEST        (1U <<   1)
#define MOD_WAIT_CONN_ACCEPT       (1U <<   2)
#define MOD_WAIT_TX_DONE           (1U <<   3)
#define MOD_WAIT_RX_DONE(n)        (1U << ( 4 + (n))) /* n == [0,4] */
#define MOD_WAIT_CONN_OPEN(n)      (1U << ( 9 + (n))) /* n == [0,4] */
#define MOD_WAIT_CONN_CLOSE(n)     (1U << (14 + (n))) /* n == [0,4] */
#define MOD_WAIT_HTTP_RESPONSE     (1U << 19) 

#define SOCK_WAIT_HTTP_RESP_COMPLETE    (1UL << 0)
#define SOCK_WAIT_HTTP_RESP_FAIL    (1UL << 1)
#define SOCK_WAIT_HTTP_RESP_PARTIAL (1UL << 2)
#define SOCK_WAIT_HTTP_RESP_PARTIAL_END (1UL << 3)



/* Socket State */
#define SOCKET_STATE_FREE           0U
#define SOCKET_STATE_CREATED        1U
#define SOCKET_STATE_BOUND          2U
#define SOCKET_STATE_LISTEN         3U
#define SOCKET_STATE_CONNECTREQ     4U
#define SOCKET_STATE_CONNECTED      5U
#define SOCKET_STATE_CLOSING        6U
#define SOCKET_STATE_CLOSED         7U
#define SOCKET_STATE_SERVER         8U

/* Socket flags */
#define SOCKET_FLAGS_NONBLOCK       (1U << 0)
#define SOCKET_FLAGS_KEEPALIVE      (1U << 1)
#define SOCKET_FLAGS_TOUT           (1U << 2)

/* Socket control block */
typedef struct {
  uint8_t *mem;
  uint32_t count;
  uint32_t mem_size;
  uint32_t remain_size;;

} MOD_CURRENT_SOCKET;

/* Socket control block */
typedef struct {
  BUF_LIST mem;                 /* Socket memory           */
  int32_t  type;                /* Socket type: TCP, DGRAM */
  int32_t  protocol;            /* Protocol type (TCP,UDP) */
  uint32_t tout_rx;             /* Rx size(recived)// in socket mode means timeout */
  uint32_t tout_tx;             /* Tx timeout              */
  uint8_t * rx_mem[2];
  MOD_CURRENT_SOCKET current;
  osEventFlagsId_t evflags_id;

  volatile
  uint8_t state;                /* Socket State            */
  uint8_t flags;                /* Socket Flags            */

  uint8_t server;               /* Server socket                */
  uint8_t backlog;              /* Next in backlog socket list  */
  volatile
  uint32_t conn_id;             /* Socket connection id              */
  uint32_t rx_len;              /* Number of bytes in current packet */

  uint16_t r_port;              /* Remote port */
  uint16_t l_port;              /* Local port  */
  uint8_t  r_ip[4];             /* Remote ip   */
  uint8_t  l_ip[4];             /* Local ip    */
  HTTP_ResponseCallback_t response_callback;
  uint32_t response_callback_size;
  volatile
  uint8_t accepted;
} MOD_SOCKET;


#define MOD_HTTP_NO_TIMEOUT   NULL

/* MODEM driver options */
typedef struct {
  uint8_t  st_ip[4];
  uint8_t  st_gateway[4];
  uint8_t  st_netmask[4];
  uint8_t  st_bssid[6];         /* BSSID of the AP to connect to */

  uint8_t  ap_mac[6];
  uint8_t  ap_ip[4];
  uint8_t  ap_gateway[4];
  uint8_t  ap_netmask[4];
  uint8_t  ap_dhcp_pool_start[4];
  uint8_t  ap_dhcp_pool_end[4];
  uint32_t ap_dhcp_lease;
} MOD_OPTIONS;

/* MODEM driver state flags */
#define MOD_FLAGS_INIT               (1U << 0)
#define MOD_FLAGS_POWER              (1U << 1)
#define MOD_FLAGS_CONN_INFO_POOLING  (1U << 2)
#define MOD_FLAGS_AP_ACTIVE          (1U << 3)
#define MOD_FLAGS_STATION_ACTIVE     (1U << 4)
#define MOD_FLAGS_STATION_CONNECTED  (1U << 5)
#define MOD_FLAGS_STATION_GOT_IP     (1U << 6)
#define MOD_FLAGS_STATION_STATIC_IP  (1U << 7)
#define MOD_FLAGS_AP_STATIC_IP       (1U << 8)
#define MOD_FLAGS_STATION_BSSID_SET  (1U << 9)
#define MOD_FLAGS_CONN_HTTP_POOLING  (1U << 10)

#define SOCKET_INVALID                0xFF
#define CONN_ID_INVALID               5

/* MODEM driver descriptor */
typedef struct {
  MOD_SignalEvent_t cb_event;    /* Event callback              */
  osThreadId_t           thread_id;   /* Data processing thread id   */
  osEventFlagsId_t       evflags_id;  /* Event flags object id       */
  osMemoryPoolId_t       mempool_id;  /* Socket memory pool id       */
  osMutexId_t            mutex_id;    /* Socket access guard         */
  osMutexId_t            memmtx_id;   /* Memory access mutex         */
  MOD_OPTIONS           options;     /* Set/GetOption value storage */
  uint32_t               lp_timer;    /* Deep sleep time in seconds  */
  uint8_t                tx_power;    /* Stored TX_POWER value       */
  uint8_t                conn_id;     /* Connection identifier state */
  uint8_t                ap_ecn;      /* AP encryption method        */
  char                   ap_pass[33]; /* AP password                 */
  uint16_t               packdump;    /* Number of dumped rx packets */
  uint16_t               flags;       /* Driver state flags          */
} MOD_CTRL;

extern MOD_DRIVER MOD_DRIVER_(MOD_DRIVER_NUMBER);

/* Static functions */
static MOD_DRIVER_VERSION    MOD_GetVersion (void);
static MOD_CAPABILITIES MOD_GetCapabilities (void);
static int32_t  MOD_Initialize (MOD_SignalEvent_t cb_event);
static int32_t  MOD_Uninitialize (void);
static int32_t  MOD_PowerControl (MOD_POWER_STATE state);
static int32_t  MOD_GetModuleInfo (char *module_info, uint32_t max_len);
static int32_t  MOD_SetOption (uint32_t interface, uint32_t option, const void *data, uint32_t len);
static int32_t  MOD_GetOption (uint32_t interface, uint32_t option, void *data, uint32_t *len);
static int32_t  MOD_Scan (MOD_SCAN_INFO_t scan_info[], uint32_t max_num);
static uint32_t MOD_IsConnected (void);
static int32_t  MOD_GetNetInfo (MOD_NET_INFO_t *net_info);
static int32_t  MOD_SocketCreate (int32_t af, int32_t type, int32_t protocol);
static int32_t  MOD_SocketBind (int32_t socket, const uint8_t *ip, uint32_t ip_len, uint16_t port);
static int32_t  MOD_SocketListen (int32_t socket, int32_t backlog);
static int32_t  MOD_SocketAccept (int32_t socket, uint8_t *ip, uint32_t *ip_len, uint16_t *port);
static int32_t  MOD_SocketConnect (int32_t socket, const uint8_t *ip, uint32_t ip_len, uint16_t port);
static int32_t  MOD_SocketRecv (int32_t socket, void *buf, uint32_t len);
static int32_t  MOD_SocketRecvFrom (int32_t socket, void *buf, uint32_t len, uint8_t *ip, uint32_t *ip_len, uint16_t *port);
static int32_t  MOD_SocketSend (int32_t socket, const void *buf, uint32_t len);
static int32_t  MOD_SocketSendTo (int32_t socket, const void *buf, uint32_t len, const uint8_t *ip, uint32_t ip_len, uint16_t port);
static int32_t  MOD_SocketGetSockName (int32_t socket, uint8_t *ip, uint32_t *ip_len, uint16_t *port);
static int32_t  MOD_SocketGetPeerName (int32_t socket, uint8_t *ip, uint32_t *ip_len, uint16_t *port);
static int32_t  MOD_SocketGetOpt (int32_t socket, int32_t opt_id, void *opt_val, uint32_t *opt_len);
static int32_t  MOD_SocketSetOpt (int32_t socket, int32_t opt_id, const void *opt_val, uint32_t opt_len);
static int32_t  MOD_SocketClose (int32_t socket);
static int32_t  MOD_SocketGetHostByName (const char *name, int32_t af, uint8_t *ip, uint32_t *ip_len);
static int32_t  MOD_Ping (const uint8_t *ip, uint32_t ip_len);
static int32_t MOD_Context(MOD_CONTEXT_CONFIG * context);
static int32_t MOD_Activate (MOD_CONTEXT_CONFIG * context, MOD_PDP_CONTEXT * pdp);
static int32_t MOD_Deactivate (int32_t socket);
static int32_t MOD_HTTP_SetOption(MOD_HTTPOption_t option, void *data);
static int32_t MOD_HTTP_GetOption(MOD_HTTPOption_t option, char *resp, uint16_t length);
static int32_t MOD_SetURL (char * url, uint32_t length, uint32_t timeout);
static int32_t MOD_HTTP (int32_t socket, MOD_HTTP_t * httpd);
static int32_t MOD_HTTP_POST_Open (int32_t socket, uint32_t data_length, uint32_t timeout, uint32_t resptime);
static int32_t MOD_HTTP_GET_Open (int32_t socket, uint32_t data_length, uint32_t timeout, uint32_t resptime);
static int32_t MOD_HTTP_Send (int32_t socket, uint8_t  *data, uint32_t len);
static int32_t MOD_HTTP_End ( int32_t socket, 
                              uint8_t * buf, 
                              HTTP_ResponseCallback_t response_callback, 
                              uint32_t len, 
                              uint32_t timeout);
static int32_t MOD_SSL_SetOption(SSL_Config_t option, uint8_t ssl_context_id, void * data);

/* Static helpers */
static void     Modem_Thread        (void *arg) __attribute__((noreturn));
static int32_t  Modem_Wait          (uint32_t event, uint32_t timeout);
static int32_t  ResetModule        (void);
static int32_t  SetupCommunication (void);
static int32_t  IsUnspecifiedIP    (const uint8_t ip[]);
static int32_t  GetCurrentMAC      (uint32_t interface, uint8_t mac[]);
static int32_t  GetCurrentIpAddr   (uint32_t interface, uint8_t ip[], uint8_t gw[], uint8_t mask[]);
static int32_t  GetCurrentDnsAddr  (uint32_t interface, uint8_t dns0[], uint8_t dns1[]);
static uint32_t GetOpt             (const void *opt_val, uint32_t opt_len);
static uint32_t SetOpt             (void *opt_val, uint32_t val, uint32_t opt_len);
static uint32_t ConnId_Alloc       (void);
static void     ConnId_Free        (uint32_t conn_id);
static void     ConnId_Accept      (uint32_t conn_id);
static int32_t  MOD_Release        (void);

#endif /* MOD_EG915U_H__ */
