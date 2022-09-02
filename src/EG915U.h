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
#ifndef EG915U_H__
#define EG915U_H__

#include "BufList.h"

#include "Modem_EG915U_Config.h"


/* AT command set version and variant used */
#ifndef AT_VERSION
#define AT_VERSION                      0x02010000
#endif
#ifndef AT_VARIANT
#define AT_VARIANT                      AT_VARIANT_EG915U
#endif

/* AT command set version definition               */
/* Version as major.minor.patch.build:  0xMMmmppbb */
#define AT_VERSION_2_1_0_0              0x02010000
#define AT_VERSION_2_0_0_0              0x02000000
#define AT_VERSION_1_9_0_0              0x01090000
#define AT_VERSION_1_8_0_0              0x01080000
#define AT_VERSION_1_7_0_0              0x01070000
#define AT_VERSION_1_6_0_0              0x01060000
#define AT_VERSION_1_5_0_0              0x01050000
#define AT_VERSION_1_4_0_0              0x01040000
#define AT_VERSION_1_3_0_0              0x01030000
#define AT_VERSION_1_2_0_0              0x01020000
#define AT_VERSION_1_1_0_0              0x01010000
#define AT_VERSION_1_0_0_0              0x01000000

/* AT command set variants */
#define AT_VARIANT_ESP                  0   /* ESP8266  */
#define AT_VARIANT_EG915U                1   /* EG915U    */
#define AT_VARIANT_WIZ                  2   /* WizFi360 */

/* Callback events codes */
#define AT_NOTIFY_EXECUTE               0  /* Serial data available, execute parser */
#define AT_NOTIFY_CONNECTED             1  /* Local station connected to an AP      */
#define AT_NOTIFY_GOT_IP                2  /* Local station got IP address          */
#define AT_NOTIFY_DISCONNECTED          3  /* Local station disconnected from an AP */
#define AT_NOTIFY_CONNECTION_OPEN       4  /* Network connection is established     */
#define AT_NOTIFY_CONNECTION_CLOSED     5  /* Network connection is closed          */
#define AT_NOTIFY_STATION_CONNECTED     6  /* Station connects to the local AP      */
#define AT_NOTIFY_STATION_DISCONNECTED  7  /* Station disconnects from the local AP */
#define AT_NOTIFY_CONNECTION_RX_INIT    8  /* Connection will start receiving data  */
#define AT_NOTIFY_CONNECTION_RX_DATA    9  /* Connection data is ready to read      */
#define AT_NOTIFY_REQUEST_TO_SEND       10 /* Request to load data to transmit      */
#define AT_NOTIFY_RESPONSE_GENERIC      11 /* Received generic command response     */
#define AT_NOTIFY_TX_DONE               12 /* Serial transmit completed             */
#define AT_NOTIFY_OUT_OF_MEMORY         13 /* Serial parser is out of memory        */
#define AT_NOTIFY_ERR_CODE              14 /* Received "ERR_CODE" response          */
#define AT_NOTIFY_READY                 15 /* The AT firmware is ready              */
#define AT_NOTIFY_HTTP_RESPONSE         16 /* The AT firmware is ready              */
#define AT_NOTIFY_HTTP_CONTENT         17 /* The AT firmware is ready              */

/**
  AT parser notify callback function.

  \param[in]  event   Callback event code (AT_NOTIFY codes)
  \param[in]  arg     Event argument
*/
extern void AT_Notify (uint32_t event, void *arg);

/* Generic responses */
#define AT_RESP_OK                  0  /* "OK"                */
#define AT_RESP_ERROR               1  /* "ERROR"             */
#define AT_RESP_FAIL                2  /* "FAIL"              */
#define AT_RESP_SEND_OK             3  /* "SEND OK"           */
#define AT_RESP_SEND_FAIL           4  /* "SEND FAIL"         */ 
#define AT_RESP_BUSY_P              5  /* "busy p..."         */
#define AT_RESP_BUSY_S              6  /* "busy s..."         */
#define AT_RESP_ALREADY_CONNECTED   7  /* "ALREADY CONNECTED" */
#define AT_RESP_MOD_CONNECTED      8  /* "MODEM CONNECTED"    */
#define AT_RESP_MOD_GOT_IP         9  /* "MODEM GOT IP"       */
#define AT_RESP_MOD_DISCONNECT    10  /* "MODEM DISCONNECT"   */
#define AT_RESP_ECHO               11  /* (echo)              */
#define AT_RESP_READY              12  /* "ready"             */
#define AT_RESP_READY2             13  /* "ready"             */
#define AT_RESP_ERR_CODE           14  /* "ERR CODE:0x..."    */
#define AT_RESP_CONNECT           AT_RESP_ALREADY_CONNECTED  /* "ERR CODE:0x..."    */
#define AT_RESP_UNKNOWN          0xFF  /* (unknown)           */

/* AT command mode */
#define AT_CMODE_QUERY              0  /* Inquiry command: AT+<x>?      */
#define AT_CMODE_SET                1  /* Set command:     AT+<x>=<...> */
#define AT_CMODE_EXEC               2  /* Execute command: AT+<x>       */
#define AT_CMODE_TEST               3  /* Test command:    AT+<x>=?     */

/* AT_DATA_CWLAP ecn encoding */
#define AT_DATA_ECN_OPEN            0
#define AT_DATA_ECN_WEP             1
#define AT_DATA_ECN_WPA_PSK         2
#define AT_DATA_ECN_WPA2_PSK        3
#define AT_DATA_ECN_WPA_WPA2_PSK    4
#define AT_DATA_ECN_WPA2_E          5

/* List AP */
typedef struct {
  uint8_t ecn;        /* Encryption */
  char    ssid[32+1]; /* SSID string */
  int8_t  rssi;       /* Signal strength */
  uint8_t mac[6];     /* MAC address */
  uint8_t ch;         /* Channel */
  uint16_t freq_offs; /* Frequency offset of AP [kHz] */
} AT_DATA_CWLAP;


typedef enum{
  SIM_MODE_READY=0,
  SIM_MODE_PUK2,
  SIM_MODE_PIN2,
  SIM_MODE_PUK,
  SIM_MODE_PIN,
}SIM_Mode_enum;


/* Configure AP */
typedef struct {
  char *ssid;         /* SSID of AP */
  char *pwd;          /* Password string, length [8:64] bytes, ASCII */
  uint8_t ch;         /* Channel ID */
  uint8_t ecn;        /* Encryption */
  uint8_t max_conn;   /* Max number of stations, range [1,8] */
  uint8_t ssid_hide;  /* 0: SSID is broadcasted (default), 1: SSID is not broadcasted */
} AT_DATA_CWSAP;

/* Connect AP */
typedef struct {
  char    ssid[32+1]; /* SSID string */
  uint8_t bssid[6];   /* BSSID: AP MAC address */
  uint8_t ch;         /* Channel */
  uint8_t rssi;       /* Signal strength */
} AT_DATA_CWJAP;

/* Link Connection */
typedef struct {
  uint8_t  link_id;       /* Connection ID */
  char     type[4];       /* Type, "TCP", "UDP" */
  uint8_t  c_s;           /* ESP runs as 0:client, 1:server */
  uint8_t  remote_ip[4];  /* Remote IP address */
  uint16_t remote_port;   /* Remote port number */
  uint16_t local_port;    /* Local port number */
} AT_DATA_LINK_CONN;

/* Device communication interface */
typedef struct {
  uint32_t baudrate;      /* Configured baud rate */
  uint8_t  databits;      /* 5:5-bit data, 6:6-bit data, 7:7-bit data, 8:8-bit data */
  uint8_t  stopbits;      /* 1:1-stop bit, 2:2-stop bits */
  uint8_t  parity;        /* 0:none, 1:odd, 2:even */
  uint8_t  flow_control;  /* 0:none, 1:RTS, 2:CTS, 3:RTS/CTS */
} AT_PARSER_COM_SERIAL;

/* Device control block */
typedef struct {
  BUF_LIST mem;         /* Parser memory buffer */
  BUF_LIST resp;        /* Response data buffer */
  uint8_t  state;       /* Parser state */
  uint8_t  cmd_sent;    /* Last command sent     */
  uint8_t  gen_resp;    /* Generic response */
  uint8_t  msg_code;    /* Message code          */
  uint8_t  ctrl_code;   /* Control code          */
  uint8_t  resp_code;   /* Response command code */
  uint8_t  resp_len;    /* Response length */
  uint8_t  rsvd[2];     /* Reserved */
  uint32_t ipd_rx;      /* Number of bytes to receive (+IPD) */
} AT_PARSER_HANDLE;


typedef enum {
  MOD_PDP_CONTEXT_TYPE_IPV4  = 1,
  MOD_PDP_CONTEXT_TYPE_IPV6,
  MOD_PDP_CONTEXT_TYPE_IPV4V6, 

} MOD_PDPContextType_t;

typedef enum {
  MOD_PDP_CONTEXT_AUTHENTICATION_NONE =  0,
  MOD_PDP_CONTEXT_AUTHENTICATION_PAP,
  MOD_PDP_CONTEXT_AUTHENTICATION_CHAP,  

} MOD_PDPContextAuthentication_t;

typedef enum {
  MOD_PDP_CONTEXT_DEACTIVATED =  0,
  MOD_PDP_CONTEXT_ACTIVATED,

} MOD_PDPContextState_t;


typedef struct{

  uint8_t                               contextID;
  MOD_PDPContextType_t                  context_type;
  uint8_t *                             APN;
  uint8_t *                             username;
  uint8_t *                             password;
  MOD_PDPContextAuthentication_t        authentication;

} MOD_CONTEXT_CONFIG;



/* Link Connection */
typedef struct {
  uint8_t               id;       /* Connection ID */
  MOD_PDPContextState_t state;       /* Integer type. The context state */
  MOD_PDPContextType_t  type;           /* Integer type. The protocol typer */
  uint8_t               remote_ip4[4];  /* Remote IP address */
  uint16_t              remote_ip6[6];   /* Remote IP6 address  */
} MOD_PDP_CONTEXT;

typedef enum {
  HTTP_OPTION_CONTEXT_ID = 0,
  HTTP_OPTION_REQUESTHEADER,
  HTTP_OPTION_RESPONSEHEADER,
  HTTP_OPTION_SSLCTXID,
  HTTP_OPTION_CONTENTTYPE,
  HTTP_OPTION_RSPOUT_AUTO,
  HTTP_OPTION_CLOSED_IND,
  HTTP_OPTION_URL,
  HTTP_OPTION_HEADER,
  HTTP_OPTION_AUTH,
  HTTP_OPTION_FORM_DATA,
  HTTP_OPTION_RESET,
} MOD_HTTPOption_t;



#define MOD_CONTEXT_CONFIG_DEFAULT 1
#define MOD_CONTEXT_CONFIG_IDMAX 7
#define MOD_CONTEXT_CONFIG_IDMIN 1



/* AT parser state */
#define AT_STATE_ANALYZE     0
#define AT_STATE_WAIT        1
#define AT_STATE_FLUSH       2
#define AT_STATE_RESP_DATA   3
#define AT_STATE_RESP_HTTP_CONTENT    4
#define AT_STATE_RESP_GEN    5
#define AT_STATE_RECV_DATA   6
#define AT_STATE_SEND_DATA   7
#define AT_STATE_RESP_CTRL   8
#define AT_STATE_RESP_ECHO   9



 typedef enum {
  SSL_PARAM_CONTEXT0 = 0,
  SSL_PARAM_CONTEXT1,
  SSL_PARAM_CONTEXT2,
  SSL_PARAM_CONTEXT3,
  SSL_PARAM_CONTEXT4,
  SSL_PARAM_CONTEXT5,

  SSL_PARAM_VERSION_SSL3 = 0,
  SSL_PARAM_VERSION_TLS1 ,
  SSL_PARAM_VERSION_TLS11 ,
  SSL_PARAM_VERSION_TLS12 ,
  SSL_PARAM_VERSION_ALL ,

  SSL_PARAM_CIPHER_TLS_RSA_WITH_AES_256_CBC_SHA                   = 0x0035,
  SSL_PARAM_CIPHER_TLS_RSA_WITH_AES_128_CBC_SHA                   = 0X002F,
  SSL_PARAM_CIPHER_TLS_RSA_WITH_RC4_128_SHA                       = 0X0005,
  SSL_PARAM_CIPHER_TLS_RSA_WITH_RC4_128_MD5                       = 0X0004,
  SSL_PARAM_CIPHER_TLS_RSA_WITH_3DES_EDE_CBC_SHA                  = 0X000A,
  SSL_PARAM_CIPHER_TLS_RSA_WITH_AES_256_CBC_SHA256                = 0X003D,
  SSL_PARAM_CIPHER_TLS_ECDH_ECDSA_WITH_RC4_128_SHA                = 0XC002,
  SSL_PARAM_CIPHER_TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA           = 0XC003,
  SSL_PARAM_CIPHER_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA            = 0XC004,
  SSL_PARAM_CIPHER_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA            = 0XC005,
  SSL_PARAM_CIPHER_TLS_ECDHE_ECDSA_WITH_RC4_128_SHA               = 0XC007,
  SSL_PARAM_CIPHER_TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA          = 0XC008,
  SSL_PARAM_CIPHER_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA           = 0XC009,
  SSL_PARAM_CIPHER_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA           = 0XC00A,
  SSL_PARAM_CIPHER_TLS_ECDHE_RSA_WITH_RC4_128_SHA                 = 0XC011,
  SSL_PARAM_CIPHER_TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA            = 0XC012,
  SSL_PARAM_CIPHER_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA             = 0XC013,
  SSL_PARAM_CIPHER_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA             = 0XC014,
  SSL_PARAM_CIPHER_TLS_ECDH_RSA_WITH_RC4_128_SHA                  = 0xC00C,
  SSL_PARAM_CIPHER_TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA             = 0XC00D,
  SSL_PARAM_CIPHER_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA              = 0XC00E,
  SSL_PARAM_CIPHER_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA              = 0XC00F,
  SSL_PARAM_CIPHER_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256        = 0XC023,
  SSL_PARAM_CIPHER_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384        = 0xC024,
  SSL_PARAM_CIPHER_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256         = 0xC025,
  SSL_PARAM_CIPHER_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384         = 0xC026,
  SSL_PARAM_CIPHER_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256          = 0XC027,
  SSL_PARAM_CIPHER_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384          = 0XC028,
  SSL_PARAM_CIPHER_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256           = 0xC029,
  SSL_PARAM_CIPHER_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384           = 0XC02A,
  SSL_PARAM_CIPHER_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256          = 0XC02F,
  SSL_PARAM_CIPHER_MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384  = 0xC030,
  SSL_PARAM_CIPHER_SUPPORT_ALL                                    = 0xFFFF,

  SSL_PARAM_NOT_IGNORE_LTIME = 0 ,
  SSL_PARAM_IGNORE_LTIME ,

  SSL_PARAM_SECLEVEL_FREE = 0 ,
  SSL_PARAM_SECLEVEL_SERVER ,
  SSL_PARAM_SECLEVEL_SERVER_CLIENT ,

  SSL_PARAM_SNI_DISABLE = 0,
  SSL_PARAM_SNI_ENABLE,

  SSL_PARAM_NOT_IGNORE_MULTICERTCHAIN_VERIFY = 0,
  SSL_PARAM_IGNORE_MULTICERTCHAIN_VERIFY,

  SSL_PARAM_NOT_IGNORE_INVALID_CERTSIGN = 0,
  SSL_PARAM_IGNORE_INVALID_CERTSIGN,


  SSL_PARAM_DTLS_DISABLE = 0,
  SSL_PARAM_DTLS_ENABLE,

  SSL_PARAM_NOT_IGNORE_CHECK_ITEM = -1,
  SSL_PARAM_IGNORE_CHECK_ITEM_0,
  SSL_PARAM_IGNORE_CHECK_ITEM_1,
  SSL_PARAM_IGNORE_CHECK_ITEM_2,
  SSL_PARAM_IGNORE_CHECK_ITEM_3,
  SSL_PARAM_IGNORE_CHECK_ITEM_4,
  SSL_PARAM_IGNORE_CHECK_ITEM_5,
  SSL_PARAM_IGNORE_CHECK_ITEM_6,
  SSL_PARAM_IGNORE_CHECK_ITEM_7,
  SSL_PARAM_IGNORE_CHECK_ITEM_8,
  SSL_PARAM_IGNORE_CHECK_ITEM_9,
  SSL_PARAM_IGNORE_CHECK_ITEM_10,

} SSL_Param_t;

  typedef enum {
  SSL_CONFIG_VERSION = 0,
  SSL_CONFIG_CIPHER_SUITE ,
  SSL_CONFIG_CACERT ,
  SSL_CONFIG_CACERTEX ,
  SSL_CONFIG_CLIENTCERT ,
  SSL_CONFIG_CLIENTKEY,
  SSL_CONFIG_SECLEVEL,
  SSL_CONFIG_IGNORE_LOCALTIME,
  SSL_CONFIG_NEGOTIATE_TIME,
  SSL_CONFIG_SNI,
  SSL_CONFIG_IGNORE_MULTI_CERTCHAIN_VERIFY,
  SSL_CONFIG_IGNORE_INVALID_CERTSIGN,
  SSL_CONFIG_DTLS,
  SSL_CONFIG_IGNORE_CERTITEM,
  
} SSL_Config_t;

/* AT parser functions */
extern int32_t AT_Parser_Initialize   (void);
extern int32_t AT_Parser_Uninitialize (void);
extern int32_t AT_Parser_GetSerialCfg (AT_PARSER_COM_SERIAL *info);
extern int32_t AT_Parser_SetSerialCfg (AT_PARSER_COM_SERIAL *info);
extern void    AT_Parser_Execute      (void);
extern void    AT_Parser_Reset        (void);

/* Command/Response functions */

/**
  Test AT startup

  Generic response is expected.

  \return
*/
extern int32_t AT_Cmd_TestAT (void);

/**
  Restarts the module

  Generic response is expected.

  \return
*/
extern int32_t AT_Cmd_Reset (void);

/**
  Check version information

  Generic response is expected.

  \return
*/
extern int32_t AT_Cmd_GetVersion (void);

/**
  Get response to GetVersion command

  \param[out] buf
  \param[in]  len
  \return
*/
extern int32_t AT_Resp_GetVersion (uint8_t *buf, uint32_t len);

/**
  Enable or disable command echo.
  
  ESP8266 can echo received commands.
  Generic response is expected.
  
  \param[in]  enable  Echo enable(1) or disable(0)
  \return
  
*/
extern int32_t AT_Cmd_Echo (uint32_t enable);

/**
  Set/Query the current UART configuration

  Supported baudrates:
  - ESP8266: in range of [110, 115200*40]
  - WizFi360: [600:921600, 1000000, 1500000, 2000000]

  \note OK response is sent first (using old UART settings) and then UART config is switched.
  
  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  baudrate
  \param[in]  databits
  \param[in]  stop_par_flowc stopbits[5:4], parity[3:2], flow control[1:0]
  \return
*/
extern int32_t AT_Cmd_ConfigUART (uint32_t at_cmode, uint32_t baudrate, uint32_t databits, uint32_t stop_par_flowc);

/**
  Get response to ConfigUART command

  \param[out] baudrate
  \param[out] databits
  \param[out] stop_par_flowc stopbits[5:4], parity[3:2], flow control[1:0]
  \return
*/
extern int32_t AT_Resp_ConfigUART (uint32_t *baudrate, uint32_t *databits, uint32_t *stop_par_flowc);

/**
  Configure the sleep modes.

  Command: SLEEP

  \note Command can be used only in Station mode. Modem-sleep is the default mode.

  \param[in]  sleep_mode  sleep mode (0: disabled, 1: Light-sleep, 2: Modem-sleep)
  \return 0: ok, -1: error
*/
extern int32_t AT_Cmd_Sleep (uint32_t at_cmode, uint32_t sleep_mode);

/**
  Get response to Sleep command.

  \param[out]   sleep_mode  Pointer to variable where the sleep mode is stored
  \return execution status
          - negative: error
          - 0: OK, response retrieved, no more data
*/
extern int32_t AT_Resp_Sleep (uint32_t *sleep_mode);

/**
  Set maximum value of RF TX power [dBm] (set only command).

  Command: RFPOWER

  \note AT query command is not available, only AT set.

  \param[in]  tx_power  power value: range [0:82], units 0.25dBm
*/
extern int32_t AT_Cmd_TxPower (uint32_t tx_power);

/**
  Set current system messages.

  Command: SYSMSG_CUR

  Bit 0: configure the message of quitting passthrough transmission
  Bit 1: configure the message of establishing a network connection

  \note Only AT set command is available.

  \param[in]  n         message configuration bit mask [0:1]
*/
extern int32_t AT_Cmd_SysMessages (uint32_t n);



/**
  Set/Query Configures the Name of Station

  Format S: AT+CWHOSTNAME=<hostname>
  Format Q: AT+CWHOSTNAME?

  Response S:
  "OK"
  "ERROR"

  Response Q: Current HostName
  
  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  hostname  the host name of the Station, the maximum length is 32 bytes.
  \return 0: OK, -1: ERROR
*/
extern int32_t AT_Cmd_HostName (uint32_t at_cmode, const char* hostname);

/**
  Get response to HostName command

  Response Q: +CWHOSTNAME:<host	name>
  Example  Q: +CWHOSTNAME:ESP_XXXXXX\r\n\r\nOK

  \param[in]  hostname  the host name of the Station, the maximum length is 32 bytes.
  \return 0: OK, -1: ERROR
*/
extern int32_t AT_Resp_HostName (char* hostname);


/**
  Set/Query connected  or  to connect to.
  
  Command: CWJAP_CUR
  
  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  ssid
  \param[in]  pwd
  \param[in]  bssid
  \return
*/
extern int32_t AT_Cmd_ConnectAP (uint32_t at_cmode, const char *ssid, const char *pwd, const uint8_t *bssid);

/**
  Response to ConnectAP command.
  
  \param[out]  ap  query result of AP to which the station is already connected

  \return - 1: connection timeout
          - 2: wrong password
          - 3: cannot find the target AP
          - 4: connection failed
*/
extern int32_t AT_Resp_ConnectAP (AT_DATA_CWJAP *ap);


/**
  Disconnect from current  (execute only command).
*/
extern int32_t AT_Cmd_DisconnectAP (void);


/**
  Configure local  (SoftAP must be active)

  Command: CWSAP_CUR

  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  cfg       AP configuration structure
  \return
*/
extern int32_t AT_Cmd_ConfigureAP (uint32_t at_cmode, AT_DATA_CWSAP *cfg);

/**
  Get response to ConfigureAP command

  \param[in]  cfg       AP configuration structure
  \return
*/
extern int32_t AT_Resp_ConfigureAP (AT_DATA_CWSAP *cfg);


/**
  Retrieve the list of all available s

  Command: CWLAP
*/
extern int32_t AT_Cmd_ListAP (void);

/**
  Get response to ListAP command
  
  \param[out]   ap    Pointer to CWLAP structure
  \param[in] 
  \return execution status
          - negative: error
          - 0:  list is empty
          - 1:  list contains more data
          
*/
extern int32_t AT_Resp_ListAP (AT_DATA_CWLAP *ap);


/**
  Set/Query the station MAC address

  Command: CIPSTAMAC_CUR

  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  mac       Pointer to 6 byte array containing MAC address
  \return 
*/
extern int32_t AT_Cmd_StationMAC (uint32_t at_cmode, const uint8_t mac[]);

/**
  Get response to StationMAC command
  
  \param[out]   mac   Pointer to 6 byte array where MAC address will be stored
  \return execution status
*/
extern int32_t AT_Resp_StationMAC (uint8_t mac[]);


/**
  Set/Query the  MAC address
  
  Command: CIPAPMAC_CUR

  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  mac       Pointer to 6 byte array containing MAC address
  \return 
*/
extern int32_t AT_Cmd_AccessPointMAC (uint32_t at_cmode, uint8_t mac[]);

/**
  Get response to AccessPointMAC command
  
  \param[out]   mac   Pointer to 6 byte array where MAC address will be stored
  \return execution status
*/
extern int32_t AT_Resp_AccessPointMAC (uint8_t mac[]);


/**
  Set/Query current IP address of the local station

  Command: CIPSTA_CUR

  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  ip        IP address
  \param[in]  gw        Gateway address
  \param[in]  mask      Netmask
  \return 
*/
extern int32_t AT_Cmd_StationIP (uint32_t at_cmode, uint8_t ip[], uint8_t gw[], uint8_t mask[]);

/**
  Get response to StationIP command

  \param[out]   addr    Pointer to 4 byte array where the address is stored
  \return execution status
          - negative: error
          - 0: address list is empty
          - 1: address list contains more data
*/
extern int32_t AT_Resp_StationIP (uint8_t addr[]);

/**
  Set/Query current IP address of the local 

  Command: CIPAP_CUR

  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  ip    IP address
  \param[in]  gw    Gateway address
  \param[in]  mask  Netmask
  \return 
*/
extern int32_t AT_Cmd_AccessPointIP (uint32_t at_cmode, uint8_t ip[], uint8_t gw[], uint8_t mask[]);

/**
  Get response to AccessPointIP command

  \param[out]   addr    Pointer to 4 byte array where the address is stored
  \return execution status
          - negative: error
          - 0: address list is empty
          - 1: address list contains more data
*/
extern int32_t AT_Resp_AccessPointIP (uint8_t addr[]);

/**
  Set user defined DNS servers

  Command: CIPDNS_CUR

  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  enable    Enable (1) or disable (0) user defined DNS servers
  \param[in]  dns0      Primary DNS server
  \param[in]  dns1      Secondary DNS server
*/
extern int32_t AT_Cmd_DNS (uint32_t at_cmode, uint32_t enable, uint8_t dns0[], uint8_t dns1[]);

#if (AT_VARIANT == AT_VARIANT_EG915U) && (AT_VERSION >= AT_VERSION_2_0_0_0)
/**
  Get response to DNS command

  \param[out]   enable  Pointer to variable where enable flag will be stored
  \param[out]   dns0    Pointer to 4 byte array where DNS 0 address will be stored
  \param[out]   dns1    Pointer to 4 byte array where DNS 1 address will be stored
  \return execution status
          - negative: error
          - 0: address list is empty
          - 1: address list contains more data
*/
extern int32_t AT_Resp_DNS (uint32_t *enable, uint8_t dns0[], uint8_t dns1[]);
#else
/**
  Get response to DNS command

  \param[out]   addr    Pointer to 4 byte array where the address is stored
  \return execution status
          - negative: error
          - 0: address list is empty
          - 1: address list contains more data
*/
extern int32_t AT_Resp_DNS (uint8_t addr[]);
#endif

/**
  Set/Query Auto-Connect to the AP

  Command: CWAUTOCONN

  Generic response is expected.

  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  enable    0:disable, 1:enable auto-connect on power-up

  \return execution status:
          0: OK, -1 on error
*/
extern int32_t AT_Cmd_AutoConnectAP (uint32_t at_cmode, uint32_t enable);

/**
  Get response to AutoConnectAP command

  \param[out]   enable  Pointer to variable the enable status is stored
  \return execution status
          - negative: error
          - 0: OK, response retrieved, no more data
*/
extern int32_t AT_Resp_AutoConnectAP (uint32_t *enable);

/**
  Retrieve the list of IP (stations) connected to  (execute only command)
  
  Command: CWLIF
*/
extern int32_t AT_Cmd_ListIP (void);

/**
  Get response to ListIP command

  \param[out] ip  Pointer to 4 byte array where IP address will be stored
  \param[out] mac Pointer to 6 byte array where MAC address will be stored
  \return execution status
          - negative: error
          - 0: list is empty
          - 1: list contains more data
*/
extern int32_t AT_Resp_ListIP (uint8_t ip[], uint8_t mac[]);

/**
  Set send data command.

  Command: CIPSEND
  
  Response: Generic response is expected

  \param[in]  at_cmode    command mode (inquiry, set, exec)
  \param[in]  link_id     connection id
  \param[in]  length      number of bytes to send
  \param[in]  remote_ip   remote IP (UDP transmission)
  \param[in]  remote_port remote port (UDP transmission)
  \return execution status:
          0: OK, -1 on error
*/
extern int32_t AT_Cmd_SendData (uint32_t at_cmode, uint32_t link_id, uint32_t length, const uint8_t remote_ip[], uint16_t remote_port);

/**
  Set/Query connection type (single, multiple connections)

  Command: CIPMUX

  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  mode      0:single connection, 1:multiple connections
  \return execution status:
          0: OK, -1 on error
*/
extern int32_t AT_Cmd_ConnectionMux (uint32_t at_cmode, uint32_t mode);

/**
  Get response to ConnectionMux command

  \param[out] mode      0:single connection, 1:multiple connections

  \return execution status
          - negative: error
          - 0: OK, response retrieved, no more data
*/
extern int32_t AT_Resp_ConnectionMux (uint32_t *mode);

/**
  Create or delete TCP server.

  Command: CIPSERVER

  Generic response is expected.

  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  mode      0:delete server, 1:create server
  \param[in]  port      port number
  \return execution status:
          0: OK, -1 on error
*/
extern int32_t AT_Cmd_TcpServer (uint32_t at_cmode, uint32_t mode, uint16_t port);

/**
  Set the maximum connection allowed by server.

  Command: CIPSERVERMAXCONN

  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  num       maximum number of clients allowed to connect
  \return execution status:
          0: OK, -1 on error
*/
extern int32_t AT_Cmd_TcpServerMaxConn (uint32_t at_cmode, uint32_t num);

/**
  Get response to TcpServerMaxConn command

  \param[in]  num       maximum number of clients allowed to connect

  \return execution status
          - negative: error
          - 0: OK, response retrieved, no more data
*/
extern int32_t AT_Resp_TcpServerMaxConn (uint32_t *num);

/**
  Enable or disable remote IP and port with +IPD.
  
  \param[in]  mode  0:do not show remote info, 1: show remote info
*/
extern int32_t AT_Cmd_RemoteInfo (uint32_t mode);

/**
  Retrieve incomming data.

  Response: +IPD,<link ID>,<len>[,<remote IP>, <remote port>]:<data>

  This response does not have CRLF terminator, <len> is the number of bytes in <data>.

  \param[out]  link_id     connection ID
  \param[out]  len         data length
  \param[out]  remote_ip   remote IP (enabled by command AT+CIPDINFO=1)
  \param[out]  remote_port remote port (enabled by command AT+CIPDINFO=1)
  

  \return 0: ok, len of data shall be read from buffer
          negative: buffer empty or packet incomplete
*/
extern int32_t AT_Resp_IPD (uint32_t *link_id, uint32_t *len, uint8_t *remote_ip, uint16_t *remote_port);

/**
  Get the connection status.
  
  Command: CIPSTATUS
  
  \param[in]  at_cmode    Command mode (exec only)
  \return execution status:
          0: OK, -1 on error
*/
extern int32_t AT_Cmd_GetStatus (uint32_t at_cmode);

/**
  Get response to GetStatus command

  \param[out] conn         connection info

  \return execution status
          - negative: error
          - 0: OK, response retrieved, no more data
*/
int32_t AT_Resp_GetStatus (AT_DATA_LINK_CONN *conn);

/**
  Resolve IP address from host name.

  Command: CIPDOMAIN

  \note Domain name string is limited to 64 characters (EG915U, ESP8266 and WizFi360).

  \param[in]  at_cmode    Command mode (inquiry, set, exec)
  \param[in]  domain      domain name string (www.xxx.com)

  \return execution status:
          0: OK, -1 on error
*/
extern int32_t AT_Cmd_DnsFunction (uint32_t at_cmode, const char *domain);

/**
  Get response to DnsFunction command

  \param[out] ip          IP address

  \return execution status
          - negative: error
          - 0: OK, response retrieved, no more data
*/
extern int32_t AT_Resp_DnsFunction (uint8_t ip[]);

/**
  Establish TCP connection.

  Command: CIPSTART

  Generic response is expected.

  \param[in]  at_cmode    Command mode (inquiry, set, exec)
  \param[in]  link_id     ID of the connection
  \param[in]  r_ip        remote ip number
  \param[in]  r_port      remote port number
  \param[in]  keep_alive  TCP keep alive, 0:disable or [1:7200] in seconds to enable keep alive
  
  \return execution status:
          0: OK, -1 on error
*/
extern int32_t AT_Cmd_ConnOpenTCP (uint32_t at_cmode, uint32_t link_id, const uint8_t r_ip[], uint16_t r_port, uint16_t keep_alive);

/**
  Establish UDP transmission.

  Command: CIPSTART

  Generic response is expected.

  \param[in]  at_cmode    Command mode (inquiry, set, exec)
  \param[in]  link_id     ID of the connection
  \param[in]  r_ip        remote ip number
  \param[in]  r_port      remote port number
  \param[in]  l_port      local port
  \param[in]  mode        UDP mode (0:dst peer entity will not change, 1:will change once, 2:allowed to change)
  
  \return execution status:
          0: OK, -1 on error
*/
extern int32_t AT_Cmd_ConnOpenUDP (uint32_t at_cmode, uint32_t link_id, const uint8_t r_ip[], uint16_t r_port, uint16_t l_port, uint32_t mode);

/**
  Close the TCP/UDP/SSL connection.
  
  Command: CIPCLOSE

  If ID of the connection is 5, all connections will be closed.
  Generic response is expected.

  \param[in]  at_cmode  Command mode (inquiry, set, exec)
  \param[in]  link_id   ID of the connection to be closed
  \return execution status:
          0: OK, -1 on error
*/
extern int32_t AT_Cmd_ConnectionClose (uint32_t at_cmode, uint32_t link_id);

/**
  Get ping response time.

  Command: PING

  Domain can be specified either by IP or by domain name string - only one should be
  specified.

  \param[in]  at_cmode    Command mode (inquiry, set, exec)
  \param[in]  ip          IP address (xxx.xxx.xxx.xxx)
  \param[in]  domain      domain name string (www.xxx.com)
  
  \return execution status:
          0: OK, -1 on error
*/
extern int32_t AT_Cmd_Ping (uint32_t at_cmode, const uint8_t ip[], const char *domain);

/**
  Get response to Ping command.

  Argument time indicates ping timeout when MSB is set ((time & 0x80000000) != 0).

  \param[out] time        ping response time in ms

  \return execution status
          - negative: error
          - 0: OK, response retrieved, no more data
*/
extern int32_t AT_Resp_Ping (uint32_t *time);

/**
  Generic response.

  \return response code, see AT_RESP codes
*/
extern int32_t AT_Resp_Generic (void);

/**
  Get +LINK_CONN response parameters (see +SYSMSG_CUR).

  +LINK_CONN:<status_type>,<link_id>,"UDP/TCP/SSL",<c/s>,<remote_ip>,
                          <remote_port>,<local_port>
*/
extern int32_t AT_Resp_LinkConn (uint32_t *status, AT_DATA_LINK_CONN *conn);

/**
  Get connection number from the <conn_id>,CONNECT and <conn_id>,CLOSED response.

  \param[out] conn_id   connection ID
  \return execution status:
          -1: no response (buffer empty)
           0: connection number retrieved
*/
extern int32_t AT_Resp_CtrlConn (uint32_t *conn_id);

/**
  Get +STA_CONNECTED and +STA_DISCONNECTED response (mac).

  +STA_CONNECTED:<sta_mac>
  +STA_DISCONNECTED"<sta_mac>
*/
extern int32_t AT_Resp_StaMac (uint8_t mac[]);

/**
  Get ERR_CODE:0x... response.

  \param[out] err_code    Pointer to 32-bit variable where error code will be stored.
  \return execution status:
          -1: no response (buffer empty)
           0: error code retrieved
*/
extern int32_t AT_Resp_ErrCode (uint32_t *err_code);

/**
  Get number of bytes that can be sent.
*/
extern uint32_t AT_Send_GetFree (void);

/**
  Send data (reply to data transmit request).
*/
extern uint32_t AT_Send_Data (const uint8_t *buf, uint32_t len);



extern int32_t AT_Cmd_SimMode (uint32_t at_cmode, char  * pin);
extern int32_t AT_Resp_SimMode (void);
extern int32_t AT_Cmd_SignalQuality ();
extern int32_t AT_Resp_SignalQuality (void);
extern int32_t AT_Resp_Activate_PDP_Context (MOD_PDP_CONTEXT * pdp) ;
extern int32_t AT_Cmd_Activate_PDP_Context (uint32_t at_cmode, uint8_t context_id) ;
extern int32_t AT_Resp_TCPIP_Context (MOD_CONTEXT_CONFIG *  context);
extern int32_t AT_Cmd_TCPIP_Context (MOD_CONTEXT_CONFIG *  context);
extern int32_t AT_Cmd_Deactivate_PDP_Context (uint8_t context_id);
extern int32_t AT_Cmd_HTTP_Config (MOD_HTTPOption_t option, void *data);
extern int32_t AT_Resp_HTTP_Config (char *resp, uint16_t length);
extern int32_t AT_Cmd_SendURL (uint32_t at_cmode, uint32_t length,  uint32_t timeout);
extern int32_t AT_Cmd_SendGET (uint32_t rsptime, uint32_t data_length,  uint32_t input_time);
extern int32_t AT_Cmd_SendPOST (uint32_t data_length,  uint32_t input_time, uint32_t rsptime);
extern int32_t AT_Resp_HTTPErrCode (uint32_t *err_code, uint32_t *httprspcode, uint32_t *content_length);
extern int32_t AT_Cmd_QHTTPREAD (uint16_t wait_time);
extern int32_t AT_RespData_HTTP (uint8_t *buf, uint32_t len);
extern int32_t AT_Cmd_SSL_Config (SSL_Config_t option, uint8_t ssl_context_id, void * data);
extern int32_t AT_Cmd_ConfigUARTRate (uint32_t at_cmode, uint32_t baudrate);


#endif /* EG915U_H__ */
