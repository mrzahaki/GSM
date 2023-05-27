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
 
 
#include "Modem_EG915U.h"
#include "Modem_EG915U_Os.h"
#include "os_tick.h"
#include <stdlib.h>

/* Driver version */
#define MOD_DRV_VERSION MOD_DRIVER_VERSION_MAJOR_MINOR(1, 0)

/* -------------------------------------------------------------------------- */

/* Number of supported simultaneus connections */
#define MOD_SOCKET_NUM         2
#define MOD_PDPSOCKET_NUM         2
#define MODEM_HTTP_DATA_CHUNK 1024

/* Array of sockets */
static MOD_SOCKET Socket[MOD_SOCKET_NUM];
static MOD_SOCKET PDPSocket[MOD_PDPSOCKET_NUM];
/* Driver control block */
static MOD_CTRL  MOD_Ctrl;
#define pCtrl   (&MOD_Ctrl)

/* Driver capabilities */
static const MOD_CAPABILITIES DriverCapabilities = {
  1U,       /* event_ap_connect    */
  1U,       /* event_ap_disconnect */
  0U,       /* event_eth_rx_frame  */
  0U,       /* bypass_mode         */
  1U,       /* ip                  */
  0U,       /* ip6                 */
  1U,       /* ping                */
  0U        /* reserved            */
};

/**
  AT parser notify callback function.

  \param[in]  event   Callback event code
  \param[in]  arg     Event argument
*/

#define RX_FLUSH_FULL			0X2
#define RX_FLUSH_PARTIAL	0X1
void AT_Notify (uint32_t event, void *arg) {
  uint32_t temp_len = 0;
  static uint8_t  rx_sock = 0;
  static uint32_t rx_num = 0;
  uint8_t rx_flush_flg = 0;

  uint32_t *u32;
  uint8_t mac[6];
  int32_t ex;
  uint8_t  n;
  uint32_t conn_id, len;
	int32_t buffer;
  uint32_t addr;
  AT_DATA_LINK_CONN conn;
  MOD_SOCKET *sock;
  uint32_t stat;

  if (event == AT_NOTIFY_EXECUTE) {
    /* Communication on-going, execute parser */
    osThreadFlagsSet (pCtrl->thread_id, MOD_THREAD_RX_DATA);
  }
  else if (event == AT_NOTIFY_TX_DONE) {
    /* Transmit completed */
    osEventFlagsSet (pCtrl->evflags_id, MOD_WAIT_TX_DONE);
  }
  else if (event == AT_NOTIFY_RESPONSE_GENERIC) {
    /* Received generic command response */
    osEventFlagsSet (pCtrl->evflags_id, MOD_WAIT_RESP_GENERIC);

  }
  else if (event == AT_NOTIFY_HTTP_CONTENT ) {
    u32 = (uint32_t *)arg;
    addr = *u32;
    *(uint32_t *)arg = 1; // true means continue current state

    /* Find socket */
    for (n = 0U; n < MOD_SOCKET_NUM; n++) {
      if (PDPSocket[n].state == SOCKET_STATE_CONNECTED) {
        /* This is our socket */
        break;
      }
    }

    ex = osEventFlagsWait (sock->evflags_id, 
                          SOCK_WAIT_HTTP_RESP_FAIL, 
                          osFlagsWaitAny, 
                          0);

    if ((ex & osFlagsError/*Flag is NULL*/) && (n != MOD_SOCKET_NUM)) {

        /* Found corresponding socket */
        sock = &PDPSocket[n];
        if(sock->rx_len){
          temp_len = ((AT_PARSER_HANDLE *)addr)->resp_len;

          

          // copy response buffer to sock-> mem
          if(sock->response_callback){
						
						// check size of the HTTP content 
            // n is the remain size of the received data
            if(temp_len >= (sock->response_callback_size - rx_num)){
				
              temp_len = sock->response_callback_size - rx_num;
              rx_flush_flg = RX_FLUSH_PARTIAL; 
            }
            if(temp_len >= (sock->rx_len - sock->tout_rx)){
              // set rx flag
				//__BKPT();
              rx_flush_flg = RX_FLUSH_FULL; 
              temp_len = sock->rx_len - sock->tout_rx; 
            }

						temp_len = (uint32_t)BufRead ((uint8_t *)sock->rx_mem[rx_sock] + rx_num, 
                                                temp_len, 
                                                &(((AT_PARSER_HANDLE *)addr)->mem));

            sock->tout_rx += temp_len;
						rx_num += temp_len;   

            switch(rx_flush_flg){
              case RX_FLUSH_FULL:

                for(;sock->tout_rx < sock->rx_len; sock->tout_rx++, rx_num++){
                  buffer = (uint32_t)BufPeekByte (&(((AT_PARSER_HANDLE *)addr)->mem));
                  if(buffer < 0){
                    break;
                  }
                  ((uint8_t *)sock->rx_mem[rx_sock])[rx_num] = buffer;
                }
                
              case RX_FLUSH_PARTIAL:   
                osEventFlagsWait (sock->evflags_id, 
                                  SOCK_WAIT_HTTP_RESP_PARTIAL_END, 
                                  osFlagsWaitAny, 
                                  osWaitForever);

                sock->current.mem_size = rx_num;
                sock->current.mem = sock->rx_mem[rx_sock];
                sock->current.remain_size = sock->rx_len - sock->tout_rx;
                sock->current.count++;
                rx_sock = ! rx_sock;
                // sock->response_callback((uint8_t *)sock->mem.mp_id, 
                //                         sock->current.count++/*counter*/, 
                //                         /*remain size*/sock->rx_len - sock->tout_rx,
                //                         /*current data size*/rx_num);
                if(sock->tout_rx >= sock->rx_len){
                  osEventFlagsSet (sock->evflags_id, SOCK_WAIT_HTTP_RESP_COMPLETE);
                  *(uint32_t *)arg = 0; // true means continue current state
                }
                else{
                  osEventFlagsSet (sock->evflags_id, SOCK_WAIT_HTTP_RESP_PARTIAL);                                        
                }
                rx_flush_flg = 0;
                rx_num = 0;
              
            }
          }
          else{

            if(temp_len >= (sock->rx_len - sock->tout_rx)){
              // set rx flag
              rx_flush_flg = RX_FLUSH_FULL; 
              temp_len = sock->rx_len - sock->tout_rx; 
            }
            sock->tout_rx += (uint32_t)BufRead ((uint8_t *)sock->rx_mem[0] + sock->tout_rx, temp_len, &(((AT_PARSER_HANDLE *)addr)->mem));
            
            if(rx_flush_flg && (sock->rx_len == sock->tout_rx)){
              *(uint32_t *)arg = 0; // true means continue current state
							osEventFlagsSet (sock->evflags_id, SOCK_WAIT_HTTP_RESP_COMPLETE);
						}
          }
        }

    }
    
  }
  else if (event == AT_NOTIFY_CONNECTION_RX_INIT) {
    /* Data packet incomming (+IPD received) */
    u32 = (uint32_t *)arg;

    /* Initialize number of bytes to receive */
    *u32 = 0U;

    /* Initialize receiving socket and number of bytes to receive */
    rx_sock = SOCKET_INVALID;
    rx_num  = 0U;

    ex = AT_Resp_IPD (&conn_id, &len, NULL, NULL);

    if (ex == 0) {
      /* Find socket */
      for (n = 0U; n < MOD_SOCKET_NUM; n++) {
        if (Socket[n].state == SOCKET_STATE_CONNECTED) {
          if (Socket[n].conn_id == conn_id) {
            /* This is our socket */
            break;
          }
        }
      }

      if (n != MOD_SOCKET_NUM) {
        /* Found corresponding socket */
        sock = &Socket[n];

        /* Check if there is enough memory to receive incomming packet */
        rx_num = BufGetFree(&sock->mem);

        if (rx_num >= (len + 2U)) {
          /* Enough space, remember receiving socket */
          rx_sock = n;

          /* Set packet header (16-bit size) */
          BufWrite ((uint8_t *)&len, 2U, &sock->mem);

          /* Return number of bytes to receive */
          *u32 = len;
        }
      }
      
      /* Set number of bytes to copy (or dump) */
      rx_num = len;
    }
  }
  else if (event == AT_NOTIFY_CONNECTION_RX_DATA) {
    /* Read source buffer address */
    u32 = (uint32_t *)arg;
    addr = *u32;

    /* Copy received data */
    if (rx_sock != SOCKET_INVALID) {
      sock = &Socket[rx_sock];

      /* Copy data */
      len = BufCopy (&sock->mem, (BUF_LIST *)addr, rx_num);
    }
    else {
      len = BufFlush (rx_num, (BUF_LIST *)addr);
    }

    rx_num -= len;

    /* Return number of bytes left to receive */
    *u32 = rx_num;

    if (rx_sock != SOCKET_INVALID) {
      /* All data received? */
      if (rx_num == 0U) {
        /* Set event flag */
        osEventFlagsSet (pCtrl->evflags_id, MOD_WAIT_RX_DONE(rx_sock));
      }
    }
  }
  else if (event == AT_NOTIFY_REQUEST_TO_SEND) {
    /* Received '>' character, device waits for data to transmit */
    osEventFlagsSet (pCtrl->evflags_id, MOD_WAIT_TX_REQUEST);
  }
  else if (event == AT_NOTIFY_CONNECTION_OPEN) {
    /* Connection is open */
    if ((pCtrl->flags & MOD_FLAGS_CONN_HTTP_POOLING) == 0U) {
      
      osEventFlagsSet (pCtrl->evflags_id, MOD_WAIT_CONN_ACCEPT);

    }
    if ((pCtrl->flags & MOD_FLAGS_CONN_INFO_POOLING) == 0U) {
      /* Retrieve connection properties (+LINK_CONN received) */
      ex = AT_Resp_LinkConn (&stat, &conn);

      if (ex == 0) {
        /* Set connection id as used */
        ConnId_Accept (conn.link_id);

        if (conn.c_s == 1U) {
          /* Device works as a server, put connection on the backlog */
          for (n = 0U; n < MOD_SOCKET_NUM; n++) {
            if (Socket[n].state == SOCKET_STATE_SERVER) {
              /* Set pointer to server socket */
              sock = &Socket[n];

              /* Find available backlog socket */
              do {
                n = Socket[n].backlog;

                if (Socket[n].state == SOCKET_STATE_LISTEN) {
                  /* Set connection id and change state */
                  Socket[n].conn_id = conn.link_id;
                  Socket[n].accepted = false;
                  Socket[n].state   = SOCKET_STATE_CONNECTED;

                  /* Copy local and remote port number */
                  Socket[n].l_port = conn.local_port;
                  Socket[n].r_port = conn.remote_port;
                  /* Copy remote ip */
                  memcpy (Socket[n].r_ip, conn.remote_ip, 4);
                  break;
                }
              }
              while (Socket[n].backlog != sock->backlog);

              break;
            }
          }

          if (n != MOD_SOCKET_NUM) {
            /* Set event */
            osEventFlagsSet (pCtrl->evflags_id, MOD_WAIT_CONN_ACCEPT);
          }
        }
        else {
          /* Device works as a client, find socket that initiated connection open */
          for (n = 0U; n < MOD_SOCKET_NUM; n++) {
            if (Socket[n].state == SOCKET_STATE_CONNECTREQ) {
              /* Check connection id */
              if (Socket[n].conn_id == conn.link_id) {
                /* Copy local and remote port number */
                Socket[n].l_port = conn.local_port;
                Socket[n].r_port = conn.remote_port;

                /* Copy remote ip */
                memcpy (Socket[n].r_ip, conn.remote_ip, 4);

                /* Socket is now connected */
                Socket[n].state = SOCKET_STATE_CONNECTED;
                break;
              }
            }
          }

          if (n != MOD_SOCKET_NUM) {
            /* Set event */
            osEventFlagsSet (pCtrl->evflags_id, MOD_WAIT_CONN_OPEN(n));
          }
        }
      }
    }
    else {
      ex = AT_Resp_CtrlConn (&conn_id);

      if (ex == 0) {
        /* Set connection id as used */
        ConnId_Accept (conn_id);

        conn.link_id = (uint8_t)conn_id;

        /* Check client sockets for initiated connection open (<link_id>,CONNECT received) */
        for (n = 0U; n < MOD_SOCKET_NUM; n++) {
          if (Socket[n].state == SOCKET_STATE_CONNECTREQ) {
            /* Check connection id */
            if (Socket[n].conn_id == conn.link_id) {
              /* Socket is now connected */
              Socket[n].state = SOCKET_STATE_CONNECTED;
              break;
            }
          }
        }

        if (n != MOD_SOCKET_NUM) {
          /* Set event */
          osEventFlagsSet (pCtrl->evflags_id, MOD_WAIT_CONN_OPEN(n));
        }
        else {
          /* Check server sockets and put connection on the backlog */
          for (n = 0U; n < MOD_SOCKET_NUM; n++) {
            if (Socket[n].state == SOCKET_STATE_SERVER) {
              /* Set pointer to server socket */
              sock = &Socket[n];

              /* Find available backlog socket */
              do {
                n = Socket[n].backlog;

                if (Socket[n].state == SOCKET_STATE_LISTEN) {
                  /* Set connection id and change state */
                  Socket[n].conn_id = conn.link_id;
                  Socket[n].state   = SOCKET_STATE_CONNECTED;
                  break;
                }
              }
              while (Socket[n].backlog != sock->backlog);

              break;
            }
          }

          if (n != MOD_SOCKET_NUM) {
            /* Set event */
            osEventFlagsSet (pCtrl->evflags_id, MOD_WAIT_CONN_ACCEPT);
          }
        }
      }
    }
  }
  else if (event == AT_NOTIFY_CONNECTION_CLOSED) {
    /* Network connection is closed */
    ex = AT_Resp_CtrlConn (&conn_id);

    if (ex == 0) {
      /* Set connection id as free */
      ConnId_Free (conn_id);

      /* Find corresponding socket and change its state */
      for (n = 0U; n < MOD_SOCKET_NUM; n++) {
        if (Socket[n].conn_id == conn_id) {
          /* Correct connection id found */
          Socket[n].conn_id = CONN_ID_INVALID;

          if (Socket[n].backlog == SOCKET_INVALID) {
            /* This is client socket */
            if (Socket[n].state == SOCKET_STATE_CLOSING) {
              /* Connection close initiated in SocketClose */
              Socket[n].state = SOCKET_STATE_FREE;
            } else {
              /* Remote peer closed the connection */
              Socket[n].state = SOCKET_STATE_CLOSED;
            }
          } else {
            if (Socket[n].state == SOCKET_STATE_CLOSING ||
              (Socket[n].state == SOCKET_STATE_CONNECTED && !Socket[n].accepted))
            {
              /* Connection close initiated in SocketClose */
              /* Listening socket, set state back to listen */
              Socket[n].state = SOCKET_STATE_LISTEN;
            } else {
              /* Remote peer closed the connection */
              Socket[n].state = SOCKET_STATE_CLOSED;
            }
          }
          break;
        }
      }

      if (n != MOD_SOCKET_NUM) {
        /* Set event */
        osEventFlagsSet (pCtrl->evflags_id, MOD_WAIT_CONN_CLOSE(n));
      }
    }
  }
  else if (event == AT_NOTIFY_STATION_CONNECTED) {
    /* Station connects to the local AP */
    ex = AT_Resp_StaMac (mac);

    pCtrl->cb_event (MOD_EVENT_CONNECT, mac);
  }
  else if (event == AT_NOTIFY_STATION_DISCONNECTED) {
    /* Station disconnects from the local AP */
    ex = AT_Resp_StaMac (mac);

    pCtrl->cb_event (MOD_EVENT_DISCONNECT, mac); 
  }
  else if (event == AT_NOTIFY_CONNECTED) {
    /* Local station connected to an AP */
    pCtrl->flags |= MOD_FLAGS_STATION_CONNECTED;
  }
  else if (event == AT_NOTIFY_HTTP_RESPONSE) {
    /* AT firmware is ready */
    osEventFlagsSet (pCtrl->evflags_id, MOD_WAIT_HTTP_RESPONSE);
  }
  else if (event == AT_NOTIFY_GOT_IP) {
    /* Local station got IP address */
    pCtrl->flags |= MOD_FLAGS_STATION_GOT_IP;
  }
  else if (event == AT_NOTIFY_DISCONNECTED) {
    /* Local station disconnected from an AP */
    pCtrl->flags &= ~(MOD_FLAGS_STATION_CONNECTED | MOD_FLAGS_STATION_GOT_IP);
  }
  else if (event == AT_NOTIFY_READY) {
    /* AT firmware is ready */
    pCtrl->cb_event (MOD_EVENT_READY, NULL);
    osEventFlagsSet (pCtrl->evflags_id, MOD_WAIT_RESP_GENERIC);
  }
  else if (event == AT_NOTIFY_ERR_CODE) {
    /* Error code received */
    ex = AT_Resp_ErrCode (&stat);
  }
  else {
    /* Out of memory? */
    if (event == AT_NOTIFY_OUT_OF_MEMORY) {
      if (arg == NULL) {
        /* Receiving socket is out of memory */
        pCtrl->packdump++;
      }
      else {
        /* Serial parser is out of memory */
        __BKPT(0);
      }
    }
  }
}


/**
  Wait for response with timeout.

  \return -2: no response, error
          -1: no response, timeout
           0: response arrived
*/
static int32_t Modem_Wait (uint32_t event, uint32_t timeout) {
  int32_t rval;
  uint32_t flags;

  if (timeout == 0U) {
    /* Operation will not time out */
    timeout = osWaitForever;
  }

  flags = osEventFlagsWait (pCtrl->evflags_id, event, osFlagsWaitAny, timeout);

  if ((flags & osFlagsError) == 0) {
    /* Got response */
    rval = 0;
  }
  else {
    if (flags == osFlagsErrorTimeout) {
      /* Timeout */
      rval = -1;
    }
    else {
      /* Internal error */
      rval = -2;
    }
  }

  return (rval);
}


/**
  MODEM thread.
*/
void Modem_Thread (void *arg) {
  int32_t ex;
  uint32_t flags;
  uint32_t tout;

  (void)arg;

  ex = AT_Parser_Initialize();

  if (ex < 0) {
    /* Parser initialization failed */
    osThreadTerminate (osThreadGetId());
  }

  /* Set pooling timeout interval */
  tout = MOD_THREAD_POOLING_TIMEOUT;

  while (1) {
    /* Wait for thread flags until timeout expires */
    flags = osThreadFlagsWait (MOD_THREAD_FLAGS, osFlagsWaitAny, tout);

    if ((flags & osFlagsError) == 0) {
      if (flags & MOD_THREAD_TERMINATE) {
        /* Uninitialize data parser (low level) */
        AT_Parser_Uninitialize ();

        /* Self-terminate */
        osThreadTerminate (osThreadGetId());
      }
    }

    AT_Parser_Execute();
  }
}

static void Modem_ThreadKick (void) {
  osThreadFlagsSet (pCtrl->thread_id, MOD_THREAD_KICK);
}


/**
  Get driver version.

  \return        \ref MOD_DRIVER_VERSION
*/
static MOD_DRIVER_VERSION MOD_GetVersion (void) {
  MOD_DRIVER_VERSION ver = { MOD_API_VERSION, MOD_DRV_VERSION };
  return (ver);
}


/**
  Get driver capabilities.

  \return        \ref MOD_CAPABILITIES
*/
static MOD_CAPABILITIES MOD_GetCapabilities (void) {
  return (DriverCapabilities);
}


/**
  Initialize Modem Module.

  \param[in]     cb_event Pointer to \ref MOD_SignalEvent_t
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
*/
static int32_t MOD_Initialize (MOD_SignalEvent_t cb_event) {
  int32_t rval;
  osThreadAttr_t     th_attr;
  osEventFlagsAttr_t ef_attr;
  osMutexAttr_t      mtx_attr;
  osMemoryPoolAttr_t mp_attr;

  rval = MOD_DRIVER_OK;


  /* Register callback event handler */
  pCtrl->cb_event = cb_event;

  if ((pCtrl->flags & MOD_FLAGS_INIT) == 0U) {
    /* Create required RTOS resources */
    mp_attr = Socket_MemPool_Attr;
    pCtrl->mempool_id = osMemoryPoolNew (SOCKET_BUFFER_BLOCK_COUNT, SOCKET_BUFFER_BLOCK_SIZE, &mp_attr);

    /* Create event flags object */
    ef_attr = Modem_EventFlags_Attr;
    pCtrl->evflags_id = osEventFlagsNew (&ef_attr);

    /* Create socket access mutex */
    mtx_attr = Socket_Mutex_Attr;
    pCtrl->mutex_id = osMutexNew (&mtx_attr);

    /* Create buffer access mutex */
    mtx_attr = BufList_Mutex_Attr;
    pCtrl->memmtx_id = osMutexNew (&mtx_attr);

    if ((pCtrl->mempool_id == NULL) ||
        (pCtrl->evflags_id == NULL) ||
        (pCtrl->mutex_id   == NULL) ||
        (pCtrl->memmtx_id  == NULL)) {
      /* Failed to create all RTOS resources */
      rval = MOD_DRIVER_ERROR;
    }

    if (rval == MOD_DRIVER_OK) {
      /* Create Modem thread */
      th_attr = Modem_Thread_Attr;
      pCtrl->thread_id = osThreadNew (Modem_Thread, NULL, &th_attr);

      /* Thread should execute, initialize communication interface and wait blocked until waken-up */
      if (osThreadGetState(pCtrl->thread_id) != osThreadBlocked) {
        /* Incorrect thread state */
        rval = MOD_DRIVER_ERROR;
      }
    }

    if (rval != MOD_DRIVER_OK) {
      /* Failed to initialize RTOS resources */
      if (pCtrl->thread_id != NULL) {
        /* Send termination flag */
        osThreadFlagsSet (pCtrl->thread_id, MOD_THREAD_TERMINATE);
      }

      /* Clean up */
      if (pCtrl->mempool_id != NULL) {
        (void)osMemoryPoolDelete (pCtrl->mempool_id);
      }

      if (pCtrl->evflags_id != NULL) {
        (void)osEventFlagsDelete (pCtrl->evflags_id);
      }

      if (pCtrl->mutex_id != NULL) {
        (void)osMutexDelete (pCtrl->mutex_id);
      }

      if (pCtrl->memmtx_id != NULL) {
        (void)osMutexDelete (pCtrl->memmtx_id);
      }
    }
    else {
      /* Successfully initialized */
      pCtrl->flags = MOD_FLAGS_INIT;
    }
  }
  
  

  return (rval);
}


/**
  De-initialize Modem Module.

  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
*/
static int32_t MOD_Uninitialize (void) {
  int32_t rval;
  uint32_t flags;

  rval = MOD_DRIVER_OK;

  if (pCtrl->thread_id != NULL) {
    flags = osThreadFlagsSet (pCtrl->thread_id, MOD_THREAD_TERMINATE);

    if ((flags & osFlagsError) != 0U) {
      /* Should never happen */
      rval = MOD_DRIVER_ERROR;
    }
  }

  if (pCtrl->evflags_id != NULL) {
    if (osEventFlagsDelete (pCtrl->evflags_id) != osOK) {
      /* Event flags delete failed */
      rval = MOD_DRIVER_ERROR;
    }
  }

  if (pCtrl->mutex_id != NULL) {
    if (osMutexDelete (pCtrl->mutex_id) != osOK) {
      /* Mutex delete failed */
      rval = MOD_DRIVER_ERROR;
    }
  }

  if (pCtrl->memmtx_id != NULL) {
    if (osMutexDelete (pCtrl->memmtx_id) != osOK) {
      /* Mutex delete failed */
      rval = MOD_DRIVER_ERROR;
    }
  }

  if (pCtrl->mempool_id != NULL) {
    if (osMemoryPoolDelete (pCtrl->mempool_id) != osOK) {
      /* Memory pool delete failed */
      rval = MOD_DRIVER_ERROR;
    }
  }

  if (rval == MOD_DRIVER_OK) {
    /* Clear resource variables */
    pCtrl->flags     = 0U;
    pCtrl->cb_event  = NULL;
    pCtrl->thread_id = NULL;
    pCtrl->mutex_id  = NULL;
  }

  return (rval);
}


/**
  Control Modem Module Power.

  \param[in]     state     Power state
                   - \ref ARM_POWER_OFF                : Power off: no operation possible
                   - \ref ARM_POWER_LOW                : Low-power mode: sleep or deep-sleep depending on \ref MOD_LP_TIMER option set
                   - \ref ARM_POWER_FULL               : Power on: full operation at maximum performance
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid state)
*/
static int32_t MOD_PowerControl (MOD_POWER_STATE state) {
  int32_t rval, ex;
  uint32_t n, t;
  osThreadId_t id;

  if ((state != MOD_POWER_OFF)  && (state != MOD_POWER_FULL) && (state != MOD_POWER_LOW)) {
    rval = MOD_DRIVER_ERROR_PARAMETER;
  }
  else {
    switch (state) {
      case MOD_POWER_OFF:
        if ((pCtrl->flags & MOD_FLAGS_INIT) == 0U) {
          /* Not initialized */
          rval = MOD_DRIVER_ERROR;
        }
        else {
          /* Clear power flag */
          pCtrl->flags &= ~MOD_FLAGS_POWER;

          rval = MOD_DRIVER_OK;

          /* Check if we need to unlock socket mutex */
          id = osMutexGetOwner(pCtrl->mutex_id);

          if (id != NULL) {
            /* Mutex is locked */
            if (osMutexRelease (pCtrl->mutex_id) != osOK) {
              /* If we can't release the mutex, will recreate it */
              if (osMutexDelete (pCtrl->mutex_id) != osOK) {
                /* Mutex delete failed */
                rval = MOD_DRIVER_ERROR;
              }
              else {
                pCtrl->mutex_id = osMutexNew (&Socket_Mutex_Attr);

                if (pCtrl->mutex_id == NULL) {
                  rval = MOD_DRIVER_ERROR;
                }
              }
            }
          }

          /* Clear socket structures */
          for (n = 0U; n < MOD_SOCKET_NUM; n++) {
            Socket[n].state = SOCKET_STATE_FREE;
          }
        }
        break;

      case MOD_POWER_LOW:
        ex = AT_Cmd_Sleep (AT_CMODE_SET, 2U/*auto uart sleep*/);
        if (ex == 0) {
          /* Wait until response arrives */
          ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);
        }
        break;

      case MOD_POWER_FULL:
        if ((pCtrl->flags & MOD_FLAGS_INIT) == 0) {
          /* Not initialized */
          rval = MOD_DRIVER_ERROR;
        }
        else if ((pCtrl->flags & MOD_FLAGS_POWER) != 0U) {
          /* Already powered */
          rval = MOD_DRIVER_OK;
        }
        else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
          /* Mutex error */
          rval = MOD_DRIVER_ERROR;
        }
        else {
          /* Configure communication channel */
          // ex = SetupCommunication();

          // if (ex == 0) {
          //   ex = ResetModule();
          // }
          //wait to get ready response
          ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT * 2);
          ex = SetupCommunication();
					
          if (ex == 0) {
            ex = AT_Cmd_SimMode (AT_CMODE_QUERY, NULL);

            if (ex == 0) {
              /* Wait until response arrives */
              ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

              if (ex == 0) {
                /* Response arrived */
                ex = AT_Resp_SimMode();
              }
            }
          }
          /*signal check*/
          if (ex == 0) {
          n =  OS_Tick_GetCount() + MOD_RESP_TIMEOUT * 40;
						while(ex < 1){
              if(OS_Tick_GetCount() > n){
                ex=-1;
                break;
              }

              ex = AT_Cmd_SignalQuality ();
              if (ex == 0) {
                /* Wait until response arrives */
                ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

                if (ex == 0) {
                  /* Response arrived */
                  ex = AT_Resp_SignalQuality();
                }
              }
              osDelay(500);
            }
          }

          // ex = GetCurrentIpAddr (MOD_INTERFACE_STATION, pCtrl->options.st_ip,
          //                                              pCtrl->options.st_gateway,
          //                                              pCtrl->options.st_netmask);

          if (ex > 1) {
            /* Disable sleep */
            ex = AT_Cmd_Sleep (AT_CMODE_SET, 0U);

            if (ex == 0) {
              /* Wait until response arrives */
              ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

              if (ex == 0) {
                /* Response arrived */
                ex = AT_Resp_Generic();
                
                if (ex == 0) {
                  /* Wait a bit before the next command is sent out */
                  osDelay(500);
                }
								else
									ex = 0; //for other platforms 
              }
            }
          }

          if (ex == 0) {
            /* Command echo disable/enable */
            ex = AT_Cmd_Echo (MOD_AT_ECHO);

            if (ex == 0) {
              /* Wait until response arrives */
              ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

              if (ex == 0) {
                /* Response arrived */
                ex = AT_Resp_Generic();
              }
            }
          }

          if (ex == 0) {
            /* Driver is powered */
            pCtrl->flags |= MOD_FLAGS_POWER;

            /* No active connections */
            pCtrl->conn_id = 0U;

            rval = MOD_DRIVER_OK;
          } else {
            rval = MOD_DRIVER_ERROR;
          }

          if (osMutexRelease (pCtrl->mutex_id) != osOK) {
            /* Mutex error, override previous return value */
            rval = MOD_DRIVER_ERROR;
          }
        }
        break;
    }
  }

  return (rval);
}


/**
  Get Module information.

  \param[out]    module_info Pointer to character buffer were info string will be returned
  \param[in]     max_len     Maximum length of string to return (including null terminator)
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (NULL module_info pointer or max_len equals to 0)
*/
static int32_t MOD_GetModuleInfo (char *module_info, uint32_t max_len) {
  int32_t rval, ex;

  if ((module_info == NULL) || (max_len == 0U)) {
    rval = MOD_DRIVER_ERROR_PARAMETER;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = MOD_DRIVER_ERROR;
  }
  else {
    rval = MOD_DRIVER_OK;
    
    ex = AT_Cmd_GetVersion();
    
    if (ex == 0) {
      /* Wait until response arrives */
      ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

      if (ex == 0) {
        /* Check response */
        ex = AT_Resp_Generic();

        if (ex == AT_RESP_OK) {
          ex = AT_Resp_GetVersion ((uint8_t *)module_info, max_len - 1U);

          /* Add string terminator */
          module_info[ex] = '\0';
        }
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = MOD_DRIVER_ERROR;
    }
  }

  return (rval);
}


/**
  Set Modem Module Options.

  \param[in]     interface Interface (0 = Station, 1 = )
  \param[in]     option    Option to set
  \param[in]     data      Pointer to data relevant to selected option
  \param[in]     len       Length of data (in bytes)
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface, NULL data pointer or len less than option specifies)
*/
static int32_t MOD_SetOption (uint32_t interface, uint32_t option, const void *data, uint32_t len) {
  int32_t  rval, ex;
  uint32_t u32;
  uint8_t  ip_0[4], ip_1[4];

  if ((interface > 1U) || (data == NULL) || (len < 4U)) {
    rval = MOD_DRIVER_ERROR_PARAMETER;
  }
  else if (pCtrl->flags == 0U) {
    /* No init */
    rval = MOD_DRIVER_ERROR;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = MOD_DRIVER_ERROR;
  }
  else {
    rval = MOD_DRIVER_ERROR;
    ex   = -1;

    switch (option) {
      default:
      case MOD_IP6_GLOBAL:                         // Station/AP Set/Get IPv6 global address;                    data = &ip6,      len = 16, uint8_t[16]
      case MOD_IP6_LINK_LOCAL:                     // Station/AP Set/Get IPv6 link local address;                data = &ip6,      len = 16, uint8_t[16]
      case MOD_IP6_SUBNET_PREFIX_LEN:              // Station/AP Set/Get IPv6 subnet prefix length;              data = &len,      len =  4, uint32_t: 1 .. 127
      case MOD_IP6_GATEWAY:                        // Station/AP Set/Get IPv6 gateway address;                   data = &ip6,      len = 16, uint8_t[16]
      case MOD_IP6_DNS1:                           // Station/AP Set/Get IPv6 primary   DNS address;             data = &ip6,      len = 16, uint8_t[16]
      case MOD_IP6_DNS2:                           // Station/AP Set/Get IPv6 secondary DNS address;             data = &ip6,      len = 16, uint8_t[16]

        rval = MOD_DRIVER_ERROR_UNSUPPORTED;
        break;

      case MOD_BSSID:                              // Station/AP Set/Get BSSID of AP to connect or of AP;        data = &bssid,    len =  6, uint8_t[6]
        if (len < 6) {
          rval = MOD_DRIVER_ERROR_PARAMETER;
        }
        else {
          if (interface == MOD_INTERFACE_STATION) {
            /* Set BSSID of the AP to connect to */
            pCtrl->flags |= MOD_FLAGS_STATION_BSSID_SET;

            /* Store BSSID into options structure */
            memcpy (pCtrl->options.st_bssid, data, 6U);
            rval = MOD_DRIVER_OK;
          } else {
            rval = MOD_DRIVER_ERROR_UNSUPPORTED;
          }
        }
        break;

      case MOD_TX_POWER:                           // Station/AP Set/Get transmit power;                         data = &power,    len =  4, uint32_t: 0 .. 20 [dBm]
        #if (AT_VARIANT == AT_VARIANT_WIZ)
          rval = MOD_DRIVER_ERROR_UNSUPPORTED;
        #else
          u32 = *(const uint32_t *)data;

          /* Store value (see GetOption) */
          pCtrl->tx_power = (uint8_t)u32;

          /* Send command */
          #if (AT_VARIANT == AT_VARIANT_EG915U) && (AT_VERSION < AT_VERSION_2_0_0_0)
          ex = AT_Cmd_TxPower (10 - (u32/2U));  /* EG915U */
          #else
          ex = AT_Cmd_TxPower (u32 * 4U);/* ESP8266, EG915U V2.0.0 */
          #endif
        #endif
        break;

      case MOD_LP_TIMER:                           // Station    Set/Get low-power deep-sleep time;              data = &time,     len =  4, uint32_t [seconds]: 0 = disable (default)
        if (interface != MOD_INTERFACE_STATION) {
          rval = MOD_DRIVER_ERROR_UNSUPPORTED;
        }
        else {
          u32 = *(const uint32_t *)data;

          pCtrl->lp_timer = u32;

          rval = MOD_DRIVER_OK;
        }
        break;

      case MOD_DTIM:                               // Station/AP Set/Get DTIM interval;                          data = &dtim,     len =  4, uint32_t [beacons]
        rval = MOD_DRIVER_ERROR_UNSUPPORTED;
        break;

      case MOD_BEACON:                             //         AP Set/Get beacon interval;                        data = &interval, len =  4, uint32_t [ms]
        rval = MOD_DRIVER_ERROR_UNSUPPORTED;
        break;

      case MOD_MAC:                                // Station/AP Set/Get MAC;                                    data = &mac,      len =  6, uint8_t[6]
        if (len < 6) {
          rval = MOD_DRIVER_ERROR_PARAMETER;
        }
        else {
          if (interface == MOD_INTERFACE_STATION) {
            /* Set station MAC */
            ex = AT_Cmd_StationMAC (AT_CMODE_SET, (const uint8_t *)data);
          } else {
            /* Set AP MAC */
            memcpy (pCtrl->options.ap_mac, data, 6);
            rval = MOD_DRIVER_OK;
          }
        }
        break;

      case MOD_IP:                                 // Station/AP Set/Get IPv4 static/assigned address;           data = &ip,       len =  4, uint8_t[4]
      case MOD_IP_SUBNET_MASK:                     // Station/AP Set/Get IPv4 subnet mask;                       data = &mask,     len =  4, uint8_t[4]
      case MOD_IP_GATEWAY:                         // Station/AP Set/Get IPv4 gateway address;                   data = &ip,       len =  4, uint8_t[4]

        if (interface == MOD_INTERFACE_STATION) {
          if      (option == MOD_IP)             { memcpy (pCtrl->options.st_ip,      data, 4); }
          else if (option == MOD_IP_SUBNET_MASK) { memcpy (pCtrl->options.st_netmask, data, 4); }
          else if (option == MOD_IP_GATEWAY)     { memcpy (pCtrl->options.st_gateway, data, 4); }
        }
        else {
          if      (option == MOD_IP)             { memcpy (pCtrl->options.ap_ip,      data, 4); }
          else if (option == MOD_IP_SUBNET_MASK) { memcpy (pCtrl->options.ap_netmask, data, 4); }
          else if (option == MOD_IP_GATEWAY)     { memcpy (pCtrl->options.ap_gateway, data, 4); }
        }
        rval = MOD_DRIVER_OK;
        break;

      case MOD_IP_DNS1:                            // Station/AP Set/Get IPv4 primary   DNS address;             data = &ip,       len =  4, uint8_t[4]
      case MOD_IP_DNS2:                            // Station/AP Set/Get IPv4 secondary DNS address;             data = &ip,       len =  4, uint8_t[4]
        ex = GetCurrentDnsAddr (interface, ip_0, ip_1);

        if (ex == 0) {
          if (option == MOD_IP_DNS1) { memcpy (ip_0, data, 4); }
          else                            { memcpy (ip_1, data, 4); }

          ex = AT_Cmd_DNS (AT_CMODE_SET, 1, ip_0, ip_1);
        }
        break;

    }

    if (ex == 0) {
      /* Wait until response arrives */
      ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

      if (ex == 0) {
        /* Check response */
        ex = AT_Resp_Generic();

        if (ex == AT_RESP_OK) {
          /* Operation completed successfully */
          rval = MOD_DRIVER_OK;
        }
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = MOD_DRIVER_ERROR;
    }
  }

  return (rval);
}


/**
  Get Modem Module Options.

  \param[in]     interface Interface (0 = Station, 1 = )
  \param[in]     option    Option to get
  \param[out]    data      Pointer to memory where data for selected option will be returned
  \param[in,out] len       Pointer to length of data (input/output)
                   - input: maximum length of data that can be returned (in bytes)
                   - output: length of returned data (in bytes)
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface, NULL data or len pointer, or *len less than option specifies)
*/
static int32_t MOD_GetOption (uint32_t interface, uint32_t option, void *data, uint32_t *len) {
  uint32_t *pu32, u32;
  uint8_t  *pu8;
  int32_t   rval, ex;
  uint8_t   ip_0[4], ip_1[4];

  if ((interface > 1U) || (data == NULL) || (*len < 4U)) {
    rval = MOD_DRIVER_ERROR_PARAMETER;
  }
  else if (pCtrl->flags == 0U) {
    /* No power */
    rval = MOD_DRIVER_ERROR;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = MOD_DRIVER_ERROR;
  }
  else {
    rval = MOD_DRIVER_ERROR;
    ex   = -1;

    switch (option) {
      default:
      case MOD_IP6_GLOBAL:                         // Station/AP Set/Get IPv6 global address;                    data = &ip6,      len = 16, uint8_t[16]
      case MOD_IP6_LINK_LOCAL:                     // Station/AP Set/Get IPv6 link local address;                data = &ip6,      len = 16, uint8_t[16]
      case MOD_IP6_SUBNET_PREFIX_LEN:              // Station/AP Set/Get IPv6 subnet prefix length;              data = &len,      len =  4, uint32_t: 1 .. 127
      case MOD_IP6_GATEWAY:                        // Station/AP Set/Get IPv6 gateway address;                   data = &ip6,      len = 16, uint8_t[16]
      case MOD_IP6_DNS1:                           // Station/AP Set/Get IPv6 primary   DNS address;             data = &ip6,      len = 16, uint8_t[16]
      case MOD_IP6_DNS2:                           // Station/AP Set/Get IPv6 secondary DNS address;             data = &ip6,      len = 16, uint8_t[16]

        rval = MOD_DRIVER_ERROR_UNSUPPORTED;
        break;

      case MOD_BSSID:                              // Station/AP Set/Get BSSID of AP to connect or of AP;        data = &bssid,    len =  6, uint8_t[6]
        if (*len < 6U) {
          rval = MOD_DRIVER_ERROR_PARAMETER;
        }
        else {
          if (interface == MOD_INTERFACE_STATION) {
            pu8 = (uint8_t *)data;

            /* Load output value */
            memcpy (pu8, pCtrl->options.st_bssid, 6U);
            *len = 6U;

            /* Update return value */
            rval = MOD_DRIVER_OK;
          }
          else {
            rval = MOD_DRIVER_ERROR_UNSUPPORTED;
          }
        }
        break;

      case MOD_TX_POWER:                           // Station/AP Set/Get transmit power;                         data = &power,    len =  4, uint32_t: 0 .. 20 [dBm]
        pu32 = (uint32_t *)data;

        /* Load value from the control block */
        *pu32 = pCtrl->tx_power;
        *len  = 4U;

        /* Update return value */
        rval = MOD_DRIVER_OK;
        break;
      
      case MOD_LP_TIMER:                           // Station    Set/Get low-power deep-sleep time;              data = &time,     len =  4, uint32_t [seconds]: 0 = disable (default)
        if (interface != MOD_INTERFACE_STATION) {
          rval = MOD_DRIVER_ERROR_UNSUPPORTED;
        }
        else {
          pu32 = (uint32_t *)data;

          /* Load value from the control block */
          *pu32 = pCtrl->lp_timer;
          *len  = 4U;

          /* Update return value */
          rval = MOD_DRIVER_OK;
        }
        break;

      case MOD_DTIM:                               // Station/AP Set/Get DTIM interval;                          data = &dtim,     len =  4, uint32_t [beacons]
        rval = MOD_DRIVER_ERROR_UNSUPPORTED;
        break;
      
      case MOD_BEACON:                             //         AP Set/Get beacon interval;                        data = &interval, len =  4, uint32_t [ms]
        rval = MOD_DRIVER_ERROR_UNSUPPORTED;
        break;

      case MOD_MAC:                                // Station/AP Set/Get MAC;                                    data = &mac,      len =  6, uint8_t[6]
        if (*len < 6U) {
          rval = MOD_DRIVER_ERROR_PARAMETER;
        }
        else {
          *len = 6U;

          if (interface == MOD_INTERFACE_STATION) {
            ex = AT_Cmd_StationMAC (AT_CMODE_QUERY, NULL);
          } else {
            memcpy (data, pCtrl->options.ap_mac, 6U);

            rval = MOD_DRIVER_OK;
          }
        }
        break;

      case MOD_IP:                                 // Station/AP Set/Get IPv4 static/assigned address;           data = &ip,       len =  4, uint8_t[4]
      case MOD_IP_SUBNET_MASK:                     // Station/AP Set/Get IPv4 subnet mask;                       data = &mask,     len =  4, uint8_t[4]
      case MOD_IP_GATEWAY:                         // Station/AP Set/Get IPv4 gateway address;                   data = &ip,       len =  4, uint8_t[4]
        if (interface == MOD_INTERFACE_STATION) {
          pu8 = (uint8_t *)data;

          if      (option == MOD_IP)             { memcpy (data, pCtrl->options.st_ip,      4); }
          else if (option == MOD_IP_SUBNET_MASK) { memcpy (data, pCtrl->options.st_netmask, 4); }
          else if (option == MOD_IP_GATEWAY)     { memcpy (data, pCtrl->options.st_gateway, 4); }
        }
        else {
          pu8 = (uint8_t *)data;

          if      (option == MOD_IP)         { memcpy (pu8, pCtrl->options.ap_ip,      4); }
          else if (option == MOD_IP_GATEWAY) { memcpy (pu8, pCtrl->options.ap_gateway, 4); }
          else                                    { memcpy (pu8, pCtrl->options.ap_netmask, 4); }
        }
        /* Skip reading response */
        ex = -1;
        rval = MOD_DRIVER_OK;
        break;

      case MOD_IP_DNS1:                            // Station/AP Set/Get IPv4 primary   DNS address;             data = &ip,       len =  4, uint8_t[4]
      case MOD_IP_DNS2:                            // Station/AP Set/Get IPv4 secondary DNS address;             data = &ip,       len =  4, uint8_t[4]
        *len = 4U;

        ex = GetCurrentDnsAddr (interface, ip_0, ip_1);

        if (ex == 0) {
          if (option == MOD_IP_DNS1) { memcpy (data, ip_0, 4); }
          else                            { memcpy (data, ip_1, 4); }

          /* Skip reading response */
          ex = -1;
          rval = MOD_DRIVER_OK;
        }
        break;

      
    }

    if (ex == 0) {
      /* Wait until response arrives */
      ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

      if (ex == 0) {
        /* Check response */
        switch (option) {
          default:
            rval = MOD_DRIVER_ERROR;
            break;

          case MOD_BSSID:                              // Station/AP Set/Get BSSID of AP to connect or of AP;        data = &bssid,    len =  6, uint8_t[6]
            /* Read AP BSSID */
            pu8 = (uint8_t *)data;

            ex = AT_Resp_AccessPointMAC (pu8);
            break;

          case MOD_MAC:                                // Station/AP Set/Get MAC;                                    data = &mac,      len =  6, uint8_t[6]
            pu8 = (uint8_t *)data;

            if (interface == MOD_INTERFACE_STATION) {
              ex = AT_Resp_StationMAC (pu8);
            } else {
              ex = AT_Resp_AccessPointMAC (pu8);
            }
            break;

          
        }

        if (ex == 0) {
          /* Operation completed successfully */
          rval = MOD_DRIVER_OK;
        }
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = MOD_DRIVER_ERROR;
    }
  }

  return (rval);
}


/**
  Scan for available networks in range.

  \param[out]    scan_info Pointer to array of MOD_SCAN_INFO_t structures where available Scan Information will be returned
  \param[in]     max_num   Maximum number of Network Information structures to return
  \return        number of MOD_SCAN_INFO_t structures returned or error code
                   - value >= 0                        : Number of MOD_SCAN_INFO_t structures returned
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (NULL scan_info pointer or max_num equal to 0)
*/
static int32_t MOD_Scan (MOD_SCAN_INFO_t scan_info[], uint32_t max_num) {
  int32_t   rval, ex;
  AT_DATA_CWLAP ap;
  uint32_t i;
  uint8_t ecn;

  if ((scan_info == NULL) || (max_num == 0U)) {
    rval = MOD_DRIVER_ERROR_PARAMETER;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = MOD_DRIVER_ERROR;
  }
  else {
    i = 0U;
    /* Get the list of available s */
    ex = AT_Cmd_ListAP();

    if (ex == 0) {
      /* Wait until response arrives */
      ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, 5*MOD_RESP_TIMEOUT);

      if (ex == 0) {
        /* Check response */
        do {
          /* Response arrived */
          ex = AT_Resp_ListAP (&ap);

          if (ex >= 0) {
            /* Return only requested number of structures (but all of them are read out) */
            if (i < max_num) {
              /* Extract info from CWLAP structure */
              strcpy (scan_info[i].ssid, ap.ssid);
              memcpy (scan_info[i].bssid, ap.mac, 6);

              /* Translate encryption info */
              switch (ap.ecn) {
                case AT_DATA_ECN_OPEN:          ecn = MOD_SECURITY_OPEN; break;
                case AT_DATA_ECN_WEP:           ecn = MOD_SECURITY_WEP;  break;
                case AT_DATA_ECN_WPA_PSK:       ecn = MOD_SECURITY_WPA;  break;
                case AT_DATA_ECN_WPA2_PSK:
                case AT_DATA_ECN_WPA_WPA2_PSK:  ecn = MOD_SECURITY_WPA2; break;
                default:
                case AT_DATA_ECN_WPA2_E:        ecn = MOD_SECURITY_UNKNOWN; break;
              }

              scan_info[i].security = ecn;
              scan_info[i].ch       = ap.ch;
              scan_info[i].rssi     = (uint8_t)ap.rssi;

              i++;
            }
          }
        }
        while (ex > 0);
      }
    }

    if (ex == 0) {
      /* Set the number of APs returned */
      rval = (int32_t)i;
    } else {
      rval = MOD_DRIVER_ERROR;
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = MOD_DRIVER_ERROR;
    }
  }

  return (rval);
}

/**
  Get station connection status.

  \return        station connection status
                   - value != 0: Station connected
                   - value = 0: Station not connected
*/
static uint32_t MOD_IsConnected (void) {
  uint32_t conn;
  uint32_t flags;

  flags = pCtrl->flags & (MOD_FLAGS_STATION_ACTIVE | MOD_FLAGS_STATION_CONNECTED);

  if (flags == (MOD_FLAGS_STATION_ACTIVE | MOD_FLAGS_STATION_CONNECTED)) {
    conn = 1U;
  } else {
    conn = 0U;
  }

  return (conn);
}


/**
  Get station Network Information.

  \param[out]    net_info  Pointer to MOD_NET_INFO_t structure where station Network Information will be returned
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed (station not connected)
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface or NULL net_info pointer)
*/
static int32_t MOD_GetNetInfo (MOD_NET_INFO_t *net_info) {
  int32_t ex, rval;
  AT_DATA_CWJAP ap;

  if (net_info == NULL) {
    rval =  MOD_DRIVER_ERROR_PARAMETER;
  }
  else if (pCtrl->flags == 0U) {
    /* No power */
    rval = MOD_DRIVER_ERROR;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = MOD_DRIVER_ERROR;
  }
  else {
    ex = AT_Cmd_ConnectAP(AT_CMODE_QUERY, NULL, NULL, NULL);

    if (ex == 0) {
      /* Wait until response arrives */
      ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

      if (ex == 0) {
        /* Check response */
        ex = AT_Resp_ConnectAP (&ap);

        if (ex == 0) {
          /* Ensure that string is terminated */
          ap.ssid[32] = '\0';
          /* Copy ssid */
          strcpy (net_info->ssid, ap.ssid);

          /* Password is stored in control block */
          pCtrl->ap_pass[32] = '\0';
          strcpy (net_info->pass, pCtrl->ap_pass);

          /* Encryption method is stored in control block */
          net_info->security = pCtrl->ap_ecn;

          /* Copy channel */
          net_info->ch = ap.ch;

          /* Convert rssi */
          net_info->rssi = ap.rssi;
        }
      }
    }

    if (ex == 0) {
      rval = MOD_DRIVER_OK;
    } else {
      rval = MOD_DRIVER_ERROR;
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = MOD_DRIVER_ERROR;
    }
  }

  return (rval);
}


/**
  Create a communication socket.

  \param[in]     af       Address family
  \param[in]     type     Socket type
  \param[in]     protocol Socket protocol
  \return        status information
                   - Socket identification number (>=0)
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument
                   - \ref ARM_SOCKET_ENOTSUP           : Operation not supported
                   - \ref ARM_SOCKET_ENOMEM            : Not enough memory
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketCreate (int32_t af, int32_t type, int32_t protocol) {
  int32_t rval;
  int32_t n;

  rval = 0;

  /* Check Address Family */
  if (af != ARM_SOCKET_AF_INET) {
    if (af == ARM_SOCKET_AF_INET6) {
      /* IPv6 socket in not supported */
      rval = ARM_SOCKET_ENOTSUP;
    } else {
      rval = ARM_SOCKET_EINVAL;
    }
  }
  else {
    /* Check socket type and protocol */
    if (type == ARM_SOCKET_SOCK_STREAM) {
      /* TCP */
      if (protocol == 0U) {
        /* Select default protocol */
        protocol = ARM_SOCKET_IPPROTO_TCP;
      }
      else {
        if (protocol != ARM_SOCKET_IPPROTO_TCP) {
          rval = ARM_SOCKET_EINVAL;
        }
      }
    }
    else if (type ) {
      /* UDP */
      if (protocol == 0U) {
        /* Select default protocol */
        protocol = ARM_SOCKET_IPPROTO_UDP;
      }
      else {
        if (protocol != ARM_SOCKET_IPPROTO_UDP) {
          rval = ARM_SOCKET_EINVAL;
        }
      }
    }
    else {
      rval = ARM_SOCKET_EINVAL;
    }
  }

  if (rval == 0) {
    if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
      /* Mutex error */
      rval = ARM_SOCKET_ERROR;
    }
    else {
      /* Allocate socket (control block) */
      for (n = 0U; n < MOD_SOCKET_NUM; n++) {
        if (Socket[n].state == SOCKET_STATE_FREE) {
          /* Found free socket, clear the socket structure */
          memset (&Socket[n], 0x00, sizeof(MOD_SOCKET));

          /* Set initial state */
          Socket[n].state    = SOCKET_STATE_CREATED;

          /* Set socket default parameters */
          Socket[n].type     = type;
          Socket[n].protocol = protocol;
          Socket[n].server   = SOCKET_INVALID;
          Socket[n].backlog  = SOCKET_INVALID;
          Socket[n].conn_id  = CONN_ID_INVALID;
          
          Socket[n].tout_rx  = 0U;
          Socket[n].tout_tx  = 0U;

          /* Setup socket memory */
          BufInit (pCtrl->mempool_id, pCtrl->memmtx_id, &Socket[n].mem);
          break;
        }
      }

      if (n == MOD_SOCKET_NUM) {
        /* Not enough memory */
        rval = ARM_SOCKET_ENOMEM;
      }
      else {
        /* Return socket number */
        rval = n;
      }

      if (osMutexRelease (pCtrl->mutex_id) != osOK) {
        /* Mutex error, override previous return value */
        rval = ARM_SOCKET_ERROR;
      }
    }
  }

  return (rval);
}


/**
  Assign a local address to a socket.

  \param[in]     socket   Socket identification number
  \param[in]     ip       Pointer to local IP address
  \param[in]     ip_len   Length of 'ip' address in bytes
  \param[in]     port     Local port number
  \return        status information
                   - 0                                 : Operation successful
                   - \ref ARM_SOCKET_ESOCK             : Invalid socket
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument (address or socket already bound)
                   - \ref ARM_SOCKET_EADDRINUSE        : Address already in use
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketBind (int32_t socket, const uint8_t *ip, uint32_t ip_len, uint16_t port) {
  int32_t  rval;
  uint32_t n;
  uint32_t i;
  MOD_SOCKET *sock;
  const uint8_t *ip_addr = ip;

  rval = 0;

  if ((socket < 0) || (socket >= MOD_SOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else if ((ip == NULL) || ((ip_len != 4U) && (ip_len != 16U))) {
    /* Invalid IP address specification */
    rval = ARM_SOCKET_EINVAL;
  }
  else if (port == 0U) {
    /* Port zero is not allowed (no autobound) */
    rval = ARM_SOCKET_EINVAL;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = ARM_SOCKET_ERROR;
  }
  else {
    sock = &Socket[socket];

    if (sock->state == SOCKET_STATE_FREE) {
      /* Socket is not created */
      rval = ARM_SOCKET_ESOCK;
    }
    else if (sock->state == SOCKET_STATE_CONNECTED) {
      /* Socket already connected */
      rval = ARM_SOCKET_EISCONN;
    }
    else if (sock->state == SOCKET_STATE_BOUND) {
      /* Socket already bound */
      rval = ARM_SOCKET_EINVAL;
    }
    else {
      /* Check if port already in use */
      for (n = 0; n < MOD_SOCKET_NUM; n++) {
        if (Socket[n].state >= SOCKET_STATE_BOUND) {
          if (Socket[n].l_port == port) {
            /* Port is already in use */
            break;
          }
        }
      }

      if (n != MOD_SOCKET_NUM) {
        /* Port already used */
        rval = ARM_SOCKET_EADDRINUSE;
      }
      else {
        /* Copy local IP address */
        for (i = 0U; i < ip_len; i++) {
          sock->l_ip[i] = ip_addr[i];
        }

        /* Set local port and change state */
        sock->l_port = port;
        sock->state  = SOCKET_STATE_BOUND;
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = ARM_SOCKET_ERROR;
    }
  }

  return (rval);
}


/**
  Listen for socket connections.

  \param[in]     socket   Socket identification number
  \param[in]     backlog  Number of connection requests that can be queued
  \return        status information
                   - 0                                 : Operation successful
                   - \ref ARM_SOCKET_ESOCK             : Invalid socket
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument (socket not bound)
                   - \ref ARM_SOCKET_ENOTSUP           : Operation not supported
                   - \ref ARM_SOCKET_EISCONN           : Socket is already connected
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketListen (int32_t socket, int32_t backlog) {
  int32_t  ex, rval;
  uint8_t n;
  MOD_SOCKET *sock;
  int32_t p, cnt;

  rval = 0;

  if ((socket < 0) || (socket >= MOD_SOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else if ((backlog <= 0) || (backlog >= MOD_SOCKET_NUM)) {
    /* Invalid backlog number (zero is not allowed) */
    rval = ARM_SOCKET_EINVAL;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = ARM_SOCKET_ERROR;
  }
  else {

    sock = &Socket[socket];

    if (sock->type != ARM_SOCKET_SOCK_STREAM) {
      /* Not stream socket */
      rval = ARM_SOCKET_ENOTSUP;
    }
    else if (sock->state == SOCKET_STATE_FREE) {
      /* Socket not created */
      rval = ARM_SOCKET_ESOCK;
    }
    else if (sock->state == SOCKET_STATE_SERVER) {
      /* Socket already in server state */
      rval = ARM_SOCKET_EINVAL;
    }
    else if (sock->state != SOCKET_STATE_BOUND) {
      /* Not bound */
      rval = ARM_SOCKET_EINVAL;
    }
    else {
      /* Check if backlog can be created */
      cnt = 0U;

      /* Count number of free sockets */
      for (n = 0U; n < MOD_SOCKET_NUM; n++) {
        if (Socket[n].state == SOCKET_STATE_FREE) {
          cnt++;
        }
      }

      if (cnt < backlog) {
        /* Backlog too large, not supported */
        rval = ARM_SOCKET_ENOTSUP;
      }
      else {
        /* Backlog can be created */

        /* Limit number of TCP server connections */
        ex = AT_Cmd_TcpServerMaxConn (AT_CMODE_SET, (uint32_t)backlog);

        if (ex == 0) {
          /* Wait until response arrives */
          ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

          if (ex == 0) {
            /* Check response */
            ex = AT_Resp_Generic();
          }
        }

        if (ex == 0) {
          /* Start TCP server */
          ex = AT_Cmd_TcpServer (AT_CMODE_SET, 1U, sock->l_port);

          if (ex == 0) {
            /* Wait until response arrives */
            ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

            if (ex == 0) {
              /* Check response */
              ex = AT_Resp_Generic();
            }
          }
        }

        if (ex != 0) {
          /* Unable to create TCP server */
          rval = ARM_SOCKET_ERROR;
        }
        else {
          /* Server successfully created */
          sock->state = SOCKET_STATE_SERVER;

          /* Create backlog */
          cnt = backlog;
          p   = socket;

          for (n = 0U; n < MOD_SOCKET_NUM; n++) {
            if (Socket[n].state == SOCKET_STATE_FREE) {
              /* Connect socket to previous in backlog chain */
              Socket[p].backlog = n;

              /* Copy server socket structure */
              memcpy (&Socket[n], sock, sizeof(MOD_SOCKET));

              /* Set socket state */
              Socket[n].state = SOCKET_STATE_LISTEN;

              /* Set server socket and backlog socket chain */
              Socket[n].server  = (uint8_t)socket;
              Socket[n].backlog = sock->backlog;

              p = n;

              cnt--;
              if (cnt == 0) {
                break;
              }
            }
          }
        }
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = ARM_SOCKET_ERROR;
    }
  }

  return (rval);
}


/**
  Accept a new connection on a socket.

  \param[in]     socket   Socket identification number
  \param[out]    ip       Pointer to buffer where address of connecting socket shall be returned (NULL for none)
  \param[in,out] ip_len   Pointer to length of 'ip' (or NULL if 'ip' is NULL)
                   - length of supplied 'ip' on input
                   - length of stored 'ip' on output
  \param[out]    port     Pointer to buffer where port of connecting socket shall be returned (NULL for none)
  \return        status information
                   - socket identification number of accepted socket (>=0)
                   - \ref ARM_SOCKET_ESOCK             : Invalid socket
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument (socket not in listen mode)
                   - \ref ARM_SOCKET_ENOTSUP           : Operation not supported (socket type does not support accepting connections)
                   - \ref ARM_SOCKET_ECONNRESET        : Connection reset by the peer
                   - \ref ARM_SOCKET_ECONNABORTED      : Connection aborted locally
                   - \ref ARM_SOCKET_EAGAIN            : Operation would block or timed out (may be called again)
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketAccept (int32_t socket, uint8_t *ip, uint32_t *ip_len, uint16_t *port) {
  int32_t ex, rval;
  int32_t n;
  MOD_SOCKET *sock;
  AT_DATA_LINK_CONN conn;

  Modem_ThreadKick();

  rval = ARM_SOCKET_ERROR;

  if ((socket < 0) || (socket >= MOD_SOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else if ((ip != NULL) && (ip_len == NULL)) {
    /* IP is specified, but no length given */
    rval = ARM_SOCKET_EINVAL;
  }
  else if ((ip == NULL) && (ip_len != NULL)) {
    /* IP not specified, but length is given */
    rval = ARM_SOCKET_EINVAL;
  }
  else if ((ip != NULL) && (ip_len != NULL) && (*ip_len < 4U)) {
    /* Specified IP is invalid */
    rval = ARM_SOCKET_EINVAL;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = ARM_SOCKET_ERROR;
  }
  else {
    sock = &Socket[socket];

    if (!sock->type ) {
      /* Accept is not supported for UDP sockets */
      rval = ARM_SOCKET_ENOTSUP;
    }
    else if (sock->state != SOCKET_STATE_SERVER) {
      /* Socket is not server socket */
      rval = ARM_SOCKET_ESOCK;
    }
    else {
      do {
        uint8_t sockFound = false;
        
        /* Check backlog for open connections */
        n = sock->backlog;
        do
        {

          if (Socket[n].state == SOCKET_STATE_CONNECTED && !Socket[n].accepted) {
            
            sockFound = true;
            
            /* We have connection active */
            if ((pCtrl->flags & MOD_FLAGS_CONN_INFO_POOLING) != 0U) {
              /* Pool for connection status, +LINK_CONN is not available */
              ex = AT_Cmd_GetStatus (AT_CMODE_EXEC);

              if (ex == 0) {
                /* Wait until response arrives */
                ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

                if (ex == 0) {
                  /* Check response */
                  do {
                    /* Response arrived */
                    ex = AT_Resp_GetStatus (&conn);
                    
                    if (ex >= 0) {
                      /* Check if structure contains information relevant for current link id */
                      if (conn.link_id == Socket[n].conn_id) {
                        /* Copy remote ip */
                        memcpy (Socket[n].r_ip, conn.remote_ip, 4);
                        /* Set remote port */
                        Socket[n].r_port = conn.remote_port;
                        /* Set local port */
                        Socket[n].l_port = conn.local_port;
                      }
                    }
                  }
                  while (ex > 0);
                }
              }
            }
            
            Socket[n].accepted = true;

            if (ip != NULL) {
              /* Copy remote ip */
              *ip_len = 4U;
              memcpy (ip, Socket[n].r_ip, 4);
            }

            if (port != NULL) {
              /* Copy remote port */
              *port = Socket[n].r_port;
            }

            /* Return socket number */
            rval = n;
          }
          n = Socket[n].backlog;
          
        } while (!sockFound && (n != sock->backlog));
        
        if (!sockFound)
        {
          /* No connection to accept */
          if (sock->flags & SOCKET_FLAGS_NONBLOCK) {
            rval = ARM_SOCKET_EAGAIN;
          }
          else {
            /* Wait until someone connects */
            ex = Modem_Wait (MOD_WAIT_CONN_ACCEPT, MOD_SOCKET_ACCEPT_TIMEOUT);

            if (ex != 0) {
              if (ex == -1) {
                /* Timeout */
                rval = ARM_SOCKET_EAGAIN;
              }
              else if (ex == -2) {
                /* Error */
                rval = ARM_SOCKET_ERROR;
              }
              break;
            }
          }
        }
      }
      while (rval == ARM_SOCKET_ERROR);
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = ARM_SOCKET_ERROR;
    }
  }

  return (rval);
}


/**
  Connect a socket to a remote host.

  \param[in]     socket   Socket identification number
  \param[in]     ip       Pointer to remote IP address
  \param[in]     ip_len   Length of 'ip' address in bytes
  \param[in]     port     Remote port number
  \return        status information
                   - 0                                 : Operation successful
                   - \ref ARM_SOCKET_ESOCK             : Invalid socket
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument
                   - \ref ARM_SOCKET_EALREADY          : Connection already in progress
                   - \ref ARM_SOCKET_EINPROGRESS       : Operation in progress
                   - \ref ARM_SOCKET_EISCONN           : Socket is connected
                   - \ref ARM_SOCKET_ECONNREFUSED      : Connection rejected by the peer
                   - \ref ARM_SOCKET_ECONNABORTED      : Connection aborted locally
                   - \ref ARM_SOCKET_EADDRINUSE        : Address already in use
                   - \ref ARM_SOCKET_ETIMEDOUT         : Operation timed out
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketConnect (int32_t socket, const uint8_t *ip, uint32_t ip_len, uint16_t port) {
  int32_t ex, rval;
  MOD_SOCKET *sock;

  rval = 0;

  if ((socket < 0) || (socket >= MOD_SOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else if ((ip == NULL) || ((ip_len != 4U) && (ip_len != 16U))) {
    rval = ARM_SOCKET_EINVAL;
  }
  else if (port == 0U) {
    /* Port zero is not allowed (no autobound) */
    rval = ARM_SOCKET_EINVAL;
  }
  else if (ip_len != 4U) {
    /* Only IPv4 is supported */
    rval = ARM_SOCKET_ENOTSUP;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = ARM_SOCKET_ERROR;
  }
  else {
    sock = &Socket[socket];

    /* Check if socket state is correct */
    switch (sock->state) {
      case SOCKET_STATE_FREE:
        rval = ARM_SOCKET_ESOCK;
        break;

      case SOCKET_STATE_LISTEN:
      case SOCKET_STATE_SERVER:
        /* Invalid state */
        rval = ARM_SOCKET_EINVAL;
        break;

      case SOCKET_STATE_CREATED:
      case SOCKET_STATE_BOUND:
        break;

      case SOCKET_STATE_CONNECTREQ:
        /* Connecting */
        rval = ARM_SOCKET_EALREADY;
        break;

      case SOCKET_STATE_CONNECTED:
        /* Already connected */
        rval = ARM_SOCKET_EISCONN;
        break;

      case SOCKET_STATE_CLOSED:
        /* Closed */
        rval = ARM_SOCKET_ETIMEDOUT;
        break;

      default:
        rval = ARM_SOCKET_ERROR;
        break;
    }

    if (rval == 0) {
      /* Copy remote ip and port */
      memcpy (sock->r_ip, ip, 4);
      sock->r_port = port;

      if (!sock->type ) {
        /* Complete binding */
        sock->state  = SOCKET_STATE_BOUND;
      }
      else {
        if (IsUnspecifiedIP (ip) != 0U) {
          /* Uspecified address is not allowed */
          rval = ARM_SOCKET_EINVAL;
        }
        else {
          /* Initiate new TCP connection */
          sock->state = SOCKET_STATE_CONNECTREQ;

          /* Get free connection id */
          sock->conn_id = ConnId_Alloc();

          /* Open connection */
          ex = AT_Cmd_ConnOpenTCP (AT_CMODE_SET, sock->conn_id, sock->r_ip, sock->r_port, 0U);

          if (ex != 0) {
            /* Should not happen normally */
            rval = ARM_SOCKET_ERROR;
          }
          else {
            /* Wait until response arrives */
            ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_CONNOPEN_TIMEOUT);

            if (ex == 0) {
              /* Check response */
              ex = AT_Resp_Generic();

              if (ex == AT_RESP_OK) {
                rval = 0;
              }
              else if (ex == AT_RESP_ALREADY_CONNECTED) {
                rval = ARM_SOCKET_EALREADY;
              }
              else {
                if (ex == AT_RESP_ERROR) {
                  rval = ARM_SOCKET_ETIMEDOUT;
                }
              }
            }
            else {
              rval = ARM_SOCKET_ERROR;
            }
          }
        }
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = ARM_SOCKET_ERROR;
    }
  }

  return (rval);
}


/**
  Receive data or check if data is available on a connected socket.

  \param[in]     socket   Socket identification number
  \param[out]    buf      Pointer to buffer where data should be stored
  \param[in]     len      Length of buffer (in bytes), set len = 0 to check if data is available
  \return        status information
                   - number of bytes received (>=0), if len != 0
                   - 0                                 : Data is available (len = 0)
                   - \ref ARM_SOCKET_ESOCK             : Invalid socket
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument (pointer to buffer or length)
                   - \ref ARM_SOCKET_ENOTCONN          : Socket is not connected
                   - \ref ARM_SOCKET_ECONNRESET        : Connection reset by the peer
                   - \ref ARM_SOCKET_ECONNABORTED      : Connection aborted locally
                   - \ref ARM_SOCKET_EAGAIN            : Operation would block or timed out (may be called again)
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketRecv (int32_t socket, void *buf, uint32_t len) {
  int32_t rval;

  rval = MOD_SocketRecvFrom (socket, buf, len, NULL, NULL, NULL);

  return (rval);
}


/**
  Receive data or check if data is available on a socket.

  \param[in]     socket   Socket identification number
  \param[out]    buf      Pointer to buffer where data should be stored
  \param[in]     len      Length of buffer (in bytes), set len = 0 to check if data is available
  \param[out]    ip       Pointer to buffer where remote source address shall be returned (NULL for none)
  \param[in,out] ip_len   Pointer to length of 'ip' (or NULL if 'ip' is NULL)
                   - length of supplied 'ip' on input
                   - length of stored 'ip' on output
  \param[out]    port     Pointer to buffer where remote source port shall be returned (NULL for none)
  \return        status information
                   - number of bytes received (>=0), if len != 0
                   - 0                                 : Data is available (len = 0)
                   - \ref ARM_SOCKET_ESOCK             : Invalid socket
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument (pointer to buffer or length)
                   - \ref ARM_SOCKET_ENOTCONN          : Socket is not connected
                   - \ref ARM_SOCKET_ECONNRESET        : Connection reset by the peer
                   - \ref ARM_SOCKET_ECONNABORTED      : Connection aborted locally
                   - \ref ARM_SOCKET_EAGAIN            : Operation would block or timed out (may be called again)
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketRecvFrom (int32_t socket, void *buf, uint32_t len, uint8_t *ip, uint32_t *ip_len, uint16_t *port) {
  int32_t ex, rval;
  MOD_SOCKET *sock;
  uint8_t *pu8 = (uint8_t *)buf;
  uint32_t n, cnt, num;

  Modem_ThreadKick();

  rval = 0;

  if ((socket < 0) || (socket >= MOD_SOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else if ((buf == NULL) && (len != 0U)) {
    /* Invalid parameters */
    rval = ARM_SOCKET_EINVAL;
  }
  else if ((ip != NULL) && (*ip_len < 4U)) {
    /* Invalid parameters */
    rval = ARM_SOCKET_EINVAL;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = ARM_SOCKET_ERROR;
  }
  else {
    sock = &Socket[socket];

    /* Check socket state */
    switch (sock->state) {
      case SOCKET_STATE_FREE:
        rval = ARM_SOCKET_ESOCK;
        break;

      case SOCKET_STATE_CLOSING:
        rval = ARM_SOCKET_EINVAL;
        break;

      case SOCKET_STATE_BOUND:
        /* DGRAM socket must be bound */
        if (!sock->type ) {
          /* Stream socket must be connected */
          rval = ARM_SOCKET_ENOTCONN;
        }
        break;

      case SOCKET_STATE_SERVER:
      case SOCKET_STATE_CREATED:
      case SOCKET_STATE_CONNECTREQ:
        /* Connecting */
        rval = ARM_SOCKET_ENOTCONN;
        break;

      case SOCKET_STATE_CONNECTED:
        /* Connected */
        break;

      case SOCKET_STATE_CLOSED:
        /* Closed */
        if (BufGetCount(&sock->mem) > 0) {
          break;
        }
        else {
          rval = ARM_SOCKET_ECONNRESET;
        }
        break;

      case SOCKET_STATE_LISTEN:
        /* Listening */
        if (BufGetCount(&sock->mem) > 0) {
          break;
        }
        else {
          rval = ARM_SOCKET_ENOTCONN;
        }
        break;

      default:
        rval = ARM_SOCKET_ERROR;
        break;
    }
    
    if (rval == 0) {
      if (!sock->type ) {
        /* Copy remote ip and port */
        if (ip != NULL) {
          memcpy (ip, sock->r_ip, 4); *ip_len = 4U;
        }
        if (port != NULL) {
          *port = sock->r_port;
        }

        if (sock->state == SOCKET_STATE_BOUND) {
          /* Start connection request */
          sock->state = SOCKET_STATE_CONNECTREQ;

          /* Get free connection id */
          sock->conn_id = ConnId_Alloc();

          /* Establish UDP connection */
          ex = AT_Cmd_ConnOpenUDP (AT_CMODE_SET, sock->conn_id, sock->r_ip,
                                                                sock->r_port,
                                                                sock->l_port,
                                                                0U);
          if (ex != 0) {
            /* Should not happen normally */
            rval = ARM_SOCKET_ERROR;
          }
          else {
            /* Wait until response arrives */
            ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

            if (ex != 0) {
              /* Response should arrive normally */
              rval = ARM_SOCKET_ERROR;
            }
            else {
              /* Check response */
              ex = AT_Resp_Generic();

              if (ex == AT_RESP_OK) {
                rval = 0;
              }
              else if (ex == AT_RESP_ALREADY_CONNECTED) {
                rval = ARM_SOCKET_EALREADY;
              }
              else {
                rval = ARM_SOCKET_ERROR;
              }
            }
          }
        }
      }
    }

    /* Initialize number of bytes read */
    n = 0;

    while (rval == 0) {
      /* Read socket buffer */
      if (sock->state != SOCKET_STATE_CONNECTED) {
          rval = ARM_SOCKET_ERROR;
          break;
      }
      if (sock->rx_len == 0) {
        /* Read packet header */
        if (BufGetCount (&sock->mem) >= 2U) {
          /* Header is in the buffer */
          if (BufRead ((uint8_t *)&sock->rx_len, 2U, &sock->mem) != 2U) {
            /* Buffer error */
            rval = ARM_SOCKET_ERROR;
          }
        }
      }

      if (sock->rx_len == 0) {
        /* No data received */
        if (sock->flags & SOCKET_FLAGS_NONBLOCK) {
          if (n != 0U) {
            /* Received less than specified buffer length */
            rval = (int32_t)n;
          }
          else {
            /* Operation would block */
            rval = ARM_SOCKET_EAGAIN;
          }
        }
        else {
          if (n != 0U) {
            /* Received less than specified buffer length */
            rval = (int32_t)n;
          }
          else {
            /* Wait for socket data until timeout */
            osMutexRelease (pCtrl->mutex_id);

            ex = Modem_Wait(MOD_WAIT_RX_DONE(socket) | MOD_WAIT_CONN_CLOSE(socket), sock->tout_rx);

            osMutexAcquire (pCtrl->mutex_id, osWaitForever);

            if (ex != 0) {
              if (ex == -1) {
                /* Timeout expired */
                rval = ARM_SOCKET_EAGAIN;
              }
              else {
                rval = ARM_SOCKET_ERROR;
              }
            }
            
            if (sock->state != SOCKET_STATE_CONNECTED) {
              rval = ARM_SOCKET_ECONNRESET;
            }
          }
        }
      }
      else if ((sock->rx_len != 0) && (len == 0)) {
        break;
      }
      else {
        /* Receive data */
        if (!sock->type ) {
          /* Datagram socket reads messages. If message is to large to fit into */
          /* specified buffer, buffer is filled and the rest is flushed.        */
          if (len < sock->rx_len) {
            cnt = len;
          } else {
            cnt = sock->rx_len;
          }

          /* Reduce cnt for number of bytes already read */
          num = cnt - n;

          n += (uint32_t)BufRead (&pu8[n], num, &sock->mem);

          if (n != cnt) {
            /* Message incomplete, wait for more data */
          }
          else {
            /* Message read out, flush any leftovers */
            if (n < sock->rx_len) {
              BufFlush (sock->rx_len - n, &sock->mem);
            }
            /* Reload packet header */
            sock->rx_len = 0U;

            /* Return number of bytes read */
            rval = (int32_t)n;
          }
        }
        else {
          /* Stream socket reads data continuously up to the buffer length or */
          /* less if not enough data.                                         */
          cnt = len - n;

          if (cnt > sock->rx_len) {
            cnt = sock->rx_len;
          }

          cnt = (uint32_t)BufRead (&pu8[n], cnt, &sock->mem);

          sock->rx_len -= cnt;
          n            += cnt;

          if (n == len) {
            /* Return number of bytes read */
            rval = (int32_t)n;
          }
        }
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = ARM_SOCKET_ERROR;
    }
  }

  return (rval);
}


/**
  Send data or check if data can be sent on a connected socket.

  \param[in]     socket   Socket identification number
  \param[in]     buf      Pointer to buffer containing data to send
  \param[in]     len      Length of data (in bytes), set len = 0 to check if data can be sent
  \return        status information
                   - number of bytes sent (>=0), if len != 0
                   - 0                                 : Data can be sent (len = 0)
                   - \ref ARM_SOCKET_ESOCK             : Invalid socket
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument (pointer to buffer or length)
                   - \ref ARM_SOCKET_ENOTCONN          : Socket is not connected
                   - \ref ARM_SOCKET_ECONNRESET        : Connection reset by the peer
                   - \ref ARM_SOCKET_ECONNABORTED      : Connection aborted locally
                   - \ref ARM_SOCKET_EAGAIN            : Operation would block or timed out (may be called again)
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketSend (int32_t socket, const void *buf, uint32_t len) {
  int32_t rval;

  rval = MOD_SocketSendTo (socket, buf, len, NULL, 0U, 0U);

  return (rval);
}


/**
  Send data or check if data can be sent on a socket.

  \param[in]     socket   Socket identification number
  \param[in]     buf      Pointer to buffer containing data to send
  \param[in]     len      Length of data (in bytes), set len = 0 to check if data can be sent
  \param[in]     ip       Pointer to remote destination IP address
  \param[in]     ip_len   Length of 'ip' address in bytes
  \param[in]     port     Remote destination port number
  \return        status information
                   - number of bytes sent (>=0), if len != 0
                   - 0                                 : Data can be sent (len = 0)
                   - \ref ARM_SOCKET_ESOCK             : Invalid socket
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument (pointer to buffer or length)
                   - \ref ARM_SOCKET_ENOTCONN          : Socket is not connected
                   - \ref ARM_SOCKET_ECONNRESET        : Connection reset by the peer
                   - \ref ARM_SOCKET_ECONNABORTED      : Connection aborted locally
                   - \ref ARM_SOCKET_EAGAIN            : Operation would block or timed out (may be called again)
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketSendTo (int32_t socket, const void *buf, uint32_t len, const uint8_t *ip, uint32_t ip_len, uint16_t port) {
  int32_t ex, rval;
  uint32_t num, cnt, n;
  MOD_SOCKET   *sock;
  const uint8_t *pu8    = (const uint8_t *)buf;
  const uint8_t *r_ip   = NULL;
  uint16_t       r_port = 0U;

  rval = 0;

  if ((socket < 0) || (socket >= MOD_SOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else if ((buf == NULL) && (len != 0U)) {
    /* Invalid parameters */
    rval = ARM_SOCKET_EINVAL;
  }
  else if ((ip != NULL) && (ip_len > 4)) {
    /* Invalid parameters */
    rval = ARM_SOCKET_ENOTSUP;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = ARM_SOCKET_ERROR;
  }
  else {
    sock = &Socket[socket];

    if (sock->state == SOCKET_STATE_FREE) {
      /* Socket not created */
      rval = ARM_SOCKET_ESOCK;
    }
    else {
      if (sock->type == ARM_SOCKET_SOCK_STREAM) {
        /* Stream socket, ignore remote IP and port */

        /* Check socket state */
        switch (sock->state) {
          case SOCKET_STATE_FREE:
            rval = ARM_SOCKET_ESOCK;
            break;

          case SOCKET_STATE_CREATED:
          case SOCKET_STATE_BOUND:
          case SOCKET_STATE_LISTEN:
          case SOCKET_STATE_CONNECTREQ:
          case SOCKET_STATE_CLOSING:
          case SOCKET_STATE_SERVER:
            /* Socket is not connected */
            rval = ARM_SOCKET_ENOTCONN;
            break;

          case SOCKET_STATE_CONNECTED:
            /* Connected */
            break;

          case SOCKET_STATE_CLOSED:
            /* Closed */
            rval = ARM_SOCKET_ECONNRESET;
            break;

          default:
            rval = ARM_SOCKET_ERROR;
            break;
        }

        if (rval == 0) {
          r_ip   = NULL;
          r_port = 0U;
        }
      }
      else {
        /* Datagram socket */
        if (ip != NULL) {
          /* Check if ip length is valid and correctly specified */
          if (ip_len != 4U) {
            rval = ARM_SOCKET_EINVAL;
          }
          else if (IsUnspecifiedIP (ip) != 0U) {
            /* Unspecified address */
            rval = ARM_SOCKET_EINVAL;
          }
          else if (port == 0) {
            /* Invalid port number */
            rval = ARM_SOCKET_EINVAL;
          }
          else {
            /* Send UDP packet using specified IP address */
            r_ip   = ip;
            r_port = port;
          }
        }
        else {
          /* IP is not provided */
          /* Send UDP packet using IP address provided at SocketConnect */
          r_ip   = sock->r_ip;
          r_port = sock->r_port;

          if (sock->state != SOCKET_STATE_CONNECTED) {
            /* If not already connected, check if connect was called previously */
            if (sock->state != SOCKET_STATE_BOUND) {
              /* Socket is "not connected" */
              rval = ARM_SOCKET_ENOTCONN;
            }
          }
        }

        if (len >= 2048) {
          /* If the message is too long to pass atomically through the underlying */
          /* protocol, the error is returned and the message is not transmitted.  */
          rval = ARM_SOCKET_ENOMEM;
        }

        if (len > AT_Send_GetFree()) {
          /* Message does not fit into buffer */
          if (sock->flags & SOCKET_FLAGS_NONBLOCK) {
            /* Nonblocking socket returns with error, message is not sent */
            rval = ARM_SOCKET_EAGAIN;
          }
        }

        if (rval == 0) {
          if (sock->state != SOCKET_STATE_CONNECTED) {
            /* Get free connection id */
            sock->conn_id = ConnId_Alloc();

            sock->state = SOCKET_STATE_CONNECTREQ;

            /* Open connection */
            ex = AT_Cmd_ConnOpenUDP (AT_CMODE_SET, sock->conn_id, r_ip,
                                                                  r_port,
                                                                  sock->l_port,
                                                                  0U);
            if (ex != 0) {
              /* Should not happen normally */
              rval = ARM_SOCKET_ERROR;
            }
            else {
              /* Wait until response arrives */
              ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

              if (ex != 0) {
                /* Response should arrive normally */
                rval = ARM_SOCKET_ERROR;
              }
              else {
                /* Check response */
                ex = AT_Resp_Generic();

                if (ex == AT_RESP_OK) {
                  rval = 0;
                }
                else if (ex == AT_RESP_ALREADY_CONNECTED) {
                  rval = ARM_SOCKET_EALREADY;
                }
                else {
                  rval = ARM_SOCKET_ERROR;
                }
              }
            }
          }
        }
      }
    }

    if (rval == 0) {
      if (!sock->type ) {
        /* 1. Message length is less than MAX (2048 bytes) */
        /* 2. For non-blocking socket, message can fit into buffer */
        /* 3. For blocking socket, message might need to be sent using multiple AT_Send_Data calls */
        if (len != 0) {
          /* Initiate send operation */
          ex = AT_Cmd_SendData (AT_CMODE_SET, sock->conn_id, len, r_ip, r_port);

          if (ex == 0) {
            /* Wait until response arrives */
            ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);
          }

          if (ex == 0) {
            /* Wait until response arrives */
            ex = Modem_Wait (MOD_WAIT_TX_REQUEST, MOD_RESP_TIMEOUT);
          }

          if (ex != 0) {
            /* Serial driver error or device not accepting data */
            rval = ARM_SOCKET_ERROR;
          }
          else {
            /* Start sending actual data to device */
            /* Set number of bytes sent */
            num = 0U;

            while (num < len) {
              /* Determine amount of data to send */
              cnt = AT_Send_GetFree();

              if (cnt == 0U) {
                /* Tx buffer full, wait until tx buffer available */
                ex = Modem_Wait (MOD_WAIT_TX_DONE, MOD_RESP_TIMEOUT);

                if (ex == 0) {
                  /* Data transfer completed */
                  cnt = AT_Send_GetFree();
                }
                else {
                  /* Device internal error */
                  rval = ARM_SOCKET_ERROR;
                  break;
                }
              }

              if (cnt > (len - num)) {
                cnt = (len - num);
              }

              osEventFlagsClear (pCtrl->evflags_id, MOD_WAIT_TX_DONE);

              num += AT_Send_Data (&pu8[num], cnt);
            }

            /* Data sent, wait for SEND OK or SEND FAIL responses */
            ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

            if (ex == 0) {
              /* Check response */
              ex = AT_Resp_Generic();

              if (ex == AT_RESP_SEND_OK) {
                /* Packet sent, return number of bytes sent */
                rval = (int32_t)num;
              }
              else if (ex == AT_RESP_SEND_FAIL) {
                /* Send failed */
                rval = ARM_SOCKET_ERROR;
              }
              else {
                /* Should not happend */
                rval = ARM_SOCKET_ERROR;
              }
            }
            else {
              /* No response? */
              rval = ARM_SOCKET_ERROR;
            }
          }
        }
      }
      else {
        /* Stream socket */
        /* 1. For non-blocking socket, we can send number of bytes that fit into tx buffer */
        /* 2. For blocking socket, we can send 2048 in one pass, but fill tx buffer multiple times */

        /* Set number of bytes sent */
        num = 0U;

        while (rval == 0) {
          if ((len == 0) && (AT_Send_GetFree() != 0)) {
            break;
          }
          /* Determine the amount of data to send */
          else if (sock->flags & SOCKET_FLAGS_NONBLOCK) {
            /* Non-blocking socket sends as much as fits into tx buffer */
            cnt = AT_Send_GetFree();
          }
          else {
            /* Blocking socket can send max 2048 bytes with one send command */
            cnt = 2048;
          }

          if (cnt > (len - num)) {
            cnt = len - num;
          }

          /* Initiate send operation */
          ex = AT_Cmd_SendData (AT_CMODE_SET, sock->conn_id, cnt, r_ip, r_port);

          if (ex == 0) {
            /* Wait until response arrives */
            ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);
          }

          if (ex == 0) {
            /* Wait until response arrives */
            ex = Modem_Wait (MOD_WAIT_TX_REQUEST, MOD_RESP_TIMEOUT);
          }

          if (ex != 0) {
            /* Serial driver error or device not accepting data */
            rval = ARM_SOCKET_ERROR;
          }
          else {
            /* Start sending actual data to device */
            while (cnt != 0) {
              /* Determine amount of data to put into tx buffer */
              n = AT_Send_GetFree();

              if (n == 0U) {
                /* Tx buffer full, wait until tx buffer available */
                ex = Modem_Wait (MOD_WAIT_TX_DONE, MOD_RESP_TIMEOUT);

                if (ex == 0) {
                  /* Data transfer completed */
                  n = AT_Send_GetFree();
                }
                else {
                  /* Device internal error */
                  rval = ARM_SOCKET_ERROR;
                  break;
                }
              }

              if (n > cnt) {
                n = cnt;
              }

              osEventFlagsClear (pCtrl->evflags_id, MOD_WAIT_TX_DONE);

              n = AT_Send_Data (&pu8[num], n);

              num += n;
              cnt -= n;
            }

            /* Data sent, wait for SEND OK or SEND FAIL responses */
            ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

            if (ex == 0) {
              /* Check response */
              ex = AT_Resp_Generic();

              if (ex == AT_RESP_SEND_OK) {
                /* Packet sent, return number of bytes sent */
                if (sock->flags & SOCKET_FLAGS_NONBLOCK) {
                  rval = (int32_t)num;
                }
                else {
                  if (num == len) {
                    rval = (int32_t)num;
                  }
                }
              }
              else if (ex == AT_RESP_SEND_FAIL) {
                /* Send failed */
                rval = ARM_SOCKET_ERROR;
              }
              else {
                /* Should not happen */
                rval = ARM_SOCKET_ERROR;
              }
            }
            else {
              /* No response? */
              rval = ARM_SOCKET_ERROR;
            }
          }
        }
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = ARM_SOCKET_ERROR;
    }
  }

  return (rval);
}


/**
  Retrieve local IP address and port of a socket.

  \param[in]     socket   Socket identification number
  \param[out]    ip       Pointer to buffer where local address shall be returned (NULL for none)
  \param[in,out] ip_len   Pointer to length of 'ip' (or NULL if 'ip' is NULL)
                   - length of supplied 'ip' on input
                   - length of stored 'ip' on output
  \param[out]    port     Pointer to buffer where local port shall be returned (NULL for none)
  \return        status information
                   - 0                                 : Operation successful
                   - \ref ARM_SOCKET_ESOCK             : Invalid socket
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument (pointer to buffer or length)
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketGetSockName (int32_t socket, uint8_t *ip, uint32_t *ip_len, uint16_t *port) {
  int32_t ex, rval;
  MOD_SOCKET *sock;
  AT_DATA_LINK_CONN conn;

  rval = 0;

  if ((socket < 0) || (socket >= MOD_SOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else if ((ip != NULL) && (ip_len == NULL)) {
    /* Invalid parameters */
    rval = ARM_SOCKET_EINVAL;
  }
  else if ((ip_len != NULL) && (*ip_len < 4U)) {
    /* Invalid parameters */
    rval = ARM_SOCKET_EINVAL;
  }
  else if ((ip != NULL) && (*ip_len >= 16U)) {
    /* Only IPv4 is supported */
    rval = ARM_SOCKET_ENOTSUP;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = ARM_SOCKET_ERROR;
  }
  else {
    sock = &Socket[socket];

    if (sock->state == SOCKET_STATE_FREE) {
      rval = ARM_SOCKET_ESOCK;
    }
    else if (sock->state < SOCKET_STATE_BOUND) {
      rval = ARM_SOCKET_EINVAL;
    }
    else {
      if (!sock->type ) {
        if (sock->state == SOCKET_STATE_BOUND) {
          /* Start connection request */
          sock->state = SOCKET_STATE_CONNECTREQ;

          /* Get free connection id */
          sock->conn_id = ConnId_Alloc();

          /* Establish UDP connection */
          ex = AT_Cmd_ConnOpenUDP (AT_CMODE_SET, sock->conn_id, sock->r_ip,
                                                                sock->r_port,
                                                                sock->l_port,
                                                                0U);
          if (ex != 0) {
            /* Should not happen normally */
            rval = ARM_SOCKET_ERROR;
          }
          else {
            /* Wait until response arrives */
            ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

            if (ex != 0) {
              /* Response should arrive normally */
              rval = ARM_SOCKET_ERROR;
            }
            else {
              /* Check response */
              ex = AT_Resp_Generic();

              if (ex == AT_RESP_OK) {
                rval = 0;
              }
              else if (ex == AT_RESP_ALREADY_CONNECTED) {
                rval = ARM_SOCKET_EALREADY;
              }
              else {
                rval = ARM_SOCKET_ERROR;
              }
            }
          }
        }
      }

      if (sock->state == SOCKET_STATE_CONNECTED) {
        /* We have connection active */
        if ((pCtrl->flags & MOD_FLAGS_CONN_INFO_POOLING) != 0U) {
          /* Pool for connection status, +LINK_CONN is not available */
          ex = AT_Cmd_GetStatus (AT_CMODE_EXEC);

          if (ex == 0) {
            /* Wait until response arrives */
            ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

            if (ex == 0) {
              /* Check response */
              do {
                /* Response arrived */
                ex = AT_Resp_GetStatus (&conn);
                
                if (ex >= 0) {
                  /* Check if structure contains information relevant for current link id */
                  if (conn.link_id == sock->conn_id) {
                    /* Copy remote ip */
                    memcpy (sock->r_ip, conn.remote_ip, 4);
                    /* Set remote port */
                    sock->r_port = conn.remote_port;
                    /* Set local port */
                    sock->l_port = conn.local_port;
                  }
                }
              }
              while (ex > 0);
            }
          }
        }
      }

      if (ip != NULL) {
        /* Set the length of the returned IP */
        *ip_len = 4U;

        /* Return socket local ip */
        memcpy (ip, sock->l_ip, 4U);
      }
      if (port != NULL) {
        /* Return socket local port */
        *port = sock->l_port;
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = ARM_SOCKET_ERROR;
    }
  }

  return (rval);
}


/**
  Retrieve remote IP address and port of a socket.

  \param[in]     socket   Socket identification number
  \param[out]    ip       Pointer to buffer where remote address shall be returned (NULL for none)
  \param[in,out] ip_len   Pointer to length of 'ip' (or NULL if 'ip' is NULL)
                   - length of supplied 'ip' on input
                   - length of stored 'ip' on output
  \param[out]    port     Pointer to buffer where remote port shall be returned (NULL for none)
  \return        status information
                   - 0                                 : Operation successful
                   - \ref ARM_SOCKET_ESOCK             : Invalid socket
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument (pointer to buffer or length)
                   - \ref ARM_SOCKET_ENOTCONN          : Socket is not connected
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketGetPeerName (int32_t socket, uint8_t *ip, uint32_t *ip_len, uint16_t *port) {
  int32_t rval;
  MOD_SOCKET *sock;

  rval = 0;

  if ((socket < 0) || (socket >= MOD_SOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else if ((ip == NULL) && (ip_len != NULL)) {
    /* Invalid parameters */
    rval = ARM_SOCKET_EINVAL;
  }
  else if ((ip != NULL) && (*ip_len < 4U)) {
    /* Invalid parameters */
    rval = ARM_SOCKET_EINVAL;
  }
  else if ((ip != NULL) && (*ip_len >= 16U)) {
    /* Only IPv4 is supported */
    rval = ARM_SOCKET_ENOTSUP;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = ARM_SOCKET_ERROR;
  }
  else {
    sock = &Socket[socket];

    if (sock->type == ARM_SOCKET_SOCK_STREAM) {
      if (sock->state == SOCKET_STATE_FREE) {
        rval = ARM_SOCKET_ESOCK;
      }
      else {
        /* Stream socket must be connected */
        if (sock->state != SOCKET_STATE_CONNECTED) {
          /* Socket is not connected */
          rval = ARM_SOCKET_ENOTCONN;
        }
      }
    }
    else {
      /* Datagram socket should have r_ip and r_port set in SocketConnect */
      if (sock->state == SOCKET_STATE_FREE) {
        rval = ARM_SOCKET_ESOCK;
      }
      else if (sock->state != SOCKET_STATE_BOUND) {
        rval = ARM_SOCKET_ENOTCONN;
      }
      else if (sock->r_port == 0) {
        /* Port zero is not allowed (connect not called) */
        rval = ARM_SOCKET_ENOTCONN;
      }
      else if (IsUnspecifiedIP (sock->r_ip) != 0U) {
        /* Remote ip was not set (connect not called) */
        rval = ARM_SOCKET_ENOTCONN;
      }
    }

    if (rval == 0) {
      /* Copy remote ip and port number */
      if (ip != NULL) {
        /* Set the length of the returned IP */
        *ip_len = 4;

        /* Return socket remote ip */
        memcpy (ip, sock->r_ip, 4);
      }

      if (port != NULL) {
        /* Return socket local port */
        *port = sock->r_port;
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = ARM_SOCKET_ERROR;
    }
  }

  return (rval);
}


/**
  Get socket option.

  \param[in]     socket   Socket identification number
  \param[in]     opt_id   Option identifier
  \param[out]    opt_val  Pointer to the buffer that will receive the option value
  \param[in,out] opt_len  Pointer to length of the option value
                   - length of buffer on input
                   - length of data on output
  \return        status information
                   - 0                                 : Operation successful
                   - \ref ARM_SOCKET_ESOCK             : Invalid socket
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument
                   - \ref ARM_SOCKET_ENOTSUP           : Operation not supported
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketGetOpt (int32_t socket, int32_t opt_id, void *opt_val, uint32_t *opt_len) {
  int32_t rval;
  MOD_SOCKET *sock;
  uint32_t val;

  rval = 0;

  if ((socket < 0) || (socket >= MOD_SOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else if ((opt_val == NULL) || (opt_len == NULL) || (*opt_len < 4U)) {
    rval = ARM_SOCKET_EINVAL;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = ARM_SOCKET_ERROR;
  }
  else {
    sock = &Socket[socket];

    if (sock->state == SOCKET_STATE_FREE) {
      /* Invalid state */
      rval = ARM_SOCKET_ESOCK;
    }
    else {
      val = 0U;

      switch (opt_id) {
        case ARM_SOCKET_IO_FIONBIO:
          /* Only set is allowed */
          rval = ARM_SOCKET_EINVAL;
          break;

        case ARM_SOCKET_SO_RCVTIMEO:
          /* Set receive timeout (in ms) */
          val = sock->tout_rx;
          break;

        case ARM_SOCKET_SO_SNDTIMEO:
          /* Set send timeout (in ms) */
          val = sock->tout_tx;
          break;

        case ARM_SOCKET_SO_KEEPALIVE:
          if ((sock->flags & SOCKET_FLAGS_KEEPALIVE) != 0) {
            /* Keep-alive messages enabled */
            val = 1U;
          } else {
            /* Keep-alive messages disabled */
            val = 0U;
          }
          break;

        case ARM_SOCKET_SO_TYPE:
          /* Set socket type */
          val = (uint32_t)sock->type;
          break;

        default:
          rval = ARM_SOCKET_EINVAL;
          break;
      }

      if (rval == 0) {
        *opt_len = SetOpt (opt_val, val, 4);
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = ARM_SOCKET_ERROR;
    }
  }

  return (rval);
}


/**
  Set socket option.

  \param[in]     socket   Socket identification number
  \param[in]     opt_id   Option identifier
  \param[in]     opt_val  Pointer to the option value
  \param[in]     opt_len  Length of the option value in bytes
  \return        status information
                   - 0                                 : Operation successful
                   - \ref ARM_SOCKET_ESOCK             : Invalid socket
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument
                   - \ref ARM_SOCKET_ENOTSUP           : Operation not supported
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketSetOpt (int32_t socket, int32_t opt_id, const void *opt_val, uint32_t opt_len) {
  int32_t rval;
  MOD_SOCKET *sock;
  uint32_t val;

  rval = 0;

  if ((socket < 0) || (socket >= MOD_SOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else if ((opt_val == NULL) || (opt_len == NULL) || (opt_len != 4U)) {
    rval = ARM_SOCKET_EINVAL;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = ARM_SOCKET_ERROR;
  }
  else {
    sock = &Socket[socket];

    if (sock->state == SOCKET_STATE_FREE) {
      /* Invalid state */
      rval = ARM_SOCKET_ESOCK;
    }
    else {
      /* Extract option value */
      val = GetOpt (opt_val, opt_len);

      switch (opt_id) {
        case ARM_SOCKET_IO_FIONBIO:
          if (val == 0U) {
            /* Disable non-blocking mode */
            sock->flags &= ~SOCKET_FLAGS_NONBLOCK;
          } else {
            /* Enable non-blocking mode */
            sock->flags |=  SOCKET_FLAGS_NONBLOCK;
          }
          break;

        case ARM_SOCKET_SO_RCVTIMEO:
          /* Set receive timeout (in ms) */
          sock->tout_rx = val;
          break;

        case ARM_SOCKET_SO_SNDTIMEO:
          /* Set send timeout (in ms) */
          sock->tout_tx = val;
          break;

        case ARM_SOCKET_SO_KEEPALIVE:
          if (val == 0U) {
            /* Disable keep-alive messages */
            sock->flags &=~ SOCKET_FLAGS_KEEPALIVE;
          } else {
            /* Enable keep-alive messages */
            sock->flags |=  SOCKET_FLAGS_KEEPALIVE;
          }
          break;

        default:
          rval = ARM_SOCKET_EINVAL;
          break;
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = ARM_SOCKET_ERROR;
    }
  }

  return (rval);
}


/**
  Close and release a socket.

  \param[in]     socket   Socket identification number
  \return        status information
                   - 0                                 : Operation successful
                   - \ref ARM_SOCKET_ESOCK             : Invalid socket
                   - \ref ARM_SOCKET_EAGAIN            : Operation would block (may be called again)
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketClose (int32_t socket) {
  int32_t ex, rval;
  uint32_t n;
  MOD_SOCKET *sock;

  Modem_ThreadKick();

  rval = 0;

  if ((socket < 0) || (socket >= MOD_SOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = ARM_SOCKET_ERROR;
  }
  else {
    /* Check if close event already waiting in rx buffer */
    Modem_Wait (MOD_WAIT_CONN_CLOSE(socket), 25);

    sock = &Socket[socket];

    if (sock->state == SOCKET_STATE_FREE) {
      rval = ARM_SOCKET_ESOCK;
    }
    else if (sock->state == SOCKET_STATE_SERVER) {
      /* Parent server socket, close all child sockets and disable server */
      ex = AT_Cmd_TcpServer (AT_CMODE_SET, 0U, 0U);

      if (ex == 0) {
        /* Wait until response arrives */
        ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);
        
        if (ex == 0) {
          ex = AT_Resp_Generic();

          if (ex == AT_RESP_OK) {
            /* Connection closed */
            sock->state = SOCKET_STATE_FREE;
          }
        }
      }

      if (ex == 0) {
        /* Close all child sockets (backlog) */
        for (n = 0U; n < MOD_SOCKET_NUM; n++) {
          if (Socket[n].server != SOCKET_INVALID) {
            Socket[n].state   = SOCKET_STATE_FREE;

            Socket[n].server  = SOCKET_INVALID;
            Socket[n].backlog = SOCKET_INVALID;
          }
        }
      }
    }
    else if ((sock->state == SOCKET_STATE_CLOSED) || (sock->state == SOCKET_STATE_CONNECTREQ)) {
      /* Socket already closed or connect in progress */
      if (sock->backlog == SOCKET_INVALID) {
        /* Client socket, state is closed */
        sock->state = SOCKET_STATE_FREE;
      } else {
        /* Listening socket, set state back to listen */
        sock->state = SOCKET_STATE_LISTEN;
      }

      /* Free socket memory */
      BufUninit (&sock->mem);
    }
    else {
      /* Close client socket */
      if ((sock->state > SOCKET_STATE_LISTEN) && (sock->state <= SOCKET_STATE_CLOSING)) {
        /* Set state to close initiated */
        sock->state = SOCKET_STATE_CLOSING;

        /* Close active connection */
        ex = AT_Cmd_ConnectionClose (AT_CMODE_SET, sock->conn_id);

        if (ex == 0) {
          /* Wait until response arrives */
          ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

          if (ex == 0) {
            ex = AT_Resp_Generic();

            if (ex == AT_RESP_OK) {
              /* Connection closed */

              /* Free socket memory */
              BufUninit (&sock->mem);
            }
          }
          else {
            /* No response */
            rval = ARM_SOCKET_ERROR;
          }
        }
        if (sock->state == SOCKET_STATE_CLOSING)
          rval = ARM_SOCKET_ERROR;
      }
      else {
        /* Non-connected socket, just mark it as free */
        sock->state = SOCKET_STATE_FREE;
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = ARM_SOCKET_ERROR;
    }
  }

  return (rval);
}


/**
  Retrieve host IP address from host name.

  \param[in]     name     Host name
  \param[in]     af       Address family
  \param[out]    ip       Pointer to buffer where resolved IP address shall be returned
  \param[in,out] ip_len   Pointer to length of 'ip'
                   - length of supplied 'ip' on input
                   - length of stored 'ip' on output
  \return        status information
                   - 0                                 : Operation successful
                   - \ref ARM_SOCKET_EINVAL            : Invalid argument
                   - \ref ARM_SOCKET_ENOTSUP           : Operation not supported
                   - \ref ARM_SOCKET_ETIMEDOUT         : Operation timed out
                   - \ref ARM_SOCKET_EHOSTNOTFOUND     : Host not found
                   - \ref ARM_SOCKET_ERROR             : Unspecified error
*/
static int32_t MOD_SocketGetHostByName (const char *name, int32_t af, uint8_t *ip, uint32_t *ip_len) {
  int32_t ex, rval;

  rval = 0;

  if ((name == NULL) || (ip == NULL) || (ip_len == NULL) || (*ip_len < 4U)) {
    rval = ARM_SOCKET_EINVAL;
  }
  else if ((af != ARM_SOCKET_AF_INET) && (af != ARM_SOCKET_AF_INET6)) {
    rval = ARM_SOCKET_EINVAL;
  }
  else if (af == ARM_SOCKET_AF_INET6) {
    rval = ARM_SOCKET_ENOTSUP;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = ARM_SOCKET_ERROR;
  }
  else {
    ex = AT_Cmd_DnsFunction (AT_CMODE_SET, name);

    if (ex != 0) {
      /* Should not happend normally */
      rval = ARM_SOCKET_ERROR;
    }
    else {
      /* Wait until response arrives */
      ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, 10*MOD_RESP_TIMEOUT);

      if (ex == 0) {
        /* Check response */
        ex = AT_Resp_Generic ();
        
        if (ex == AT_RESP_OK) {
          *ip_len = 4U;
          (void)AT_Resp_DnsFunction (ip);
        }
        else {
          rval = ARM_SOCKET_EHOSTNOTFOUND;
        }
      }
      else {
        /* NOTE: need a proper way to handle timeout response */
        rval = ARM_SOCKET_EHOSTNOTFOUND;
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = ARM_SOCKET_ERROR;
    }
  }

  return (rval);
}


/**
  Probe remote host with Ping command.

  \param[in]     ip       Pointer to remote host IP address
  \param[in]     ip_len   Length of 'ip' address in bytes
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_TIMEOUT     : Timeout occurred
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (NULL ip pointer or ip_len different than 4 or 16)
*/
static int32_t MOD_Ping (const uint8_t *ip, uint32_t ip_len) {
  int32_t ex, rval;
  uint32_t val;

  rval = MOD_DRIVER_OK;

  if ((ip == NULL) || ((ip_len != 4U) && (ip_len != 16U))) {
    rval = MOD_DRIVER_ERROR_PARAMETER;
  }
  else if (ip_len == 16U) {
    rval = MOD_DRIVER_ERROR_UNSUPPORTED;
  }
  else if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = MOD_DRIVER_ERROR;
  }
  else {
    ex = AT_Cmd_Ping (AT_CMODE_SET, ip, NULL);

    if (ex == 0) {
      /* Wait until response arrives */
      ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

      if (ex == 0) {
        /* Check response */
        ex = AT_Resp_Ping (&val);

        if (ex == 0) {
          if ((val & 0x80000000) != 0) {
            /* Ping timeout */
            rval = MOD_DRIVER_ERROR_TIMEOUT;
          }
        }
      }
    }

    if (ex != 0) {
      rval = MOD_DRIVER_ERROR;
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = MOD_DRIVER_ERROR;
    }
  }

  return (rval);
}
/**
  Get and set PDP context status/configuration.

  \param[in]     context Interface settings.(in getter mode only context->id must be set 
                  (can be null to auto replace with MOD_CONTEXT_CONFIG_DEFAULT ))
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_TIMEOUT     : Timeout occurred
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported (security type, channel autodetect or WPS not supported)
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface, NULL config pointer or invalid configuration)
*/

static int32_t MOD_Context(MOD_CONTEXT_CONFIG * context){
  int32_t  ex, rval = 0;

  if (!context) {
    rval = MOD_DRIVER_ERROR_PARAMETER;
  }
  else if (context->contextID > MOD_CONTEXT_CONFIG_IDMAX && context->contextID < MOD_CONTEXT_CONFIG_IDMIN) {
    rval = MOD_DRIVER_ERROR_UNSUPPORTED;
  }
  else if (context->context_type) {
      if(!context->APN)
        rval = MOD_DRIVER_ERROR_PARAMETER;
      
    }
  
  if(context->contextID == NULL)
      context->contextID = MOD_CONTEXT_CONFIG_DEFAULT;


  if (rval == 0 || osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = MOD_DRIVER_ERROR;
  }
  else{
    rval = MOD_DRIVER_ERROR;

    ex = -1;
      
      rval = AT_Cmd_TCPIP_Context (context);

      if (rval == 0) {
        /* Wait until response arrives */
        ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT * 5);

        if (ex == 0) {
          /* Response arrived */
          if((context->context_type && context->APN) || (context->password && context->username) || context->authentication)
            ex = AT_Resp_Generic();
          else
            AT_Resp_TCPIP_Context (context); 
            
        }
      }
      
      if (osMutexRelease (pCtrl->mutex_id) != osOK) {
        /* Mutex error, override previous return value */
        rval = MOD_DRIVER_ERROR;
      }
    }

  if (ex != AT_RESP_OK) {

    if (ex == 1) {
      /* Connection timeout */
      rval = MOD_DRIVER_ERROR_TIMEOUT;
    }
    else if (ex == 2) {
      /* Wrong password */
      rval = MOD_DRIVER_ERROR;
    }
    else if (ex == 3) {
      /* Cannot find the target AP */
      /* Wrong SSID? Is AP down?   */
      rval = MOD_DRIVER_ERROR;
    }
    else {
      /* Connection failed, reason unknown */
      rval = MOD_DRIVER_ERROR;
    }
  }

  return (rval);

}

/**
  Activate interface.

  \param[in]     context Interface settings.(can set MOD_ACTIVATE_DEFAULT active default 1 context)
  \param[out]    pdp activated interface parameters.(optional can be MOD_ACTIVATE_DEFAULT value)
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_TIMEOUT     : Timeout occurred
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported (security type, channel autodetect or WPS not supported)
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface, NULL config pointer or invalid configuration)
*/
static int32_t MOD_Activate (MOD_CONTEXT_CONFIG * context, MOD_PDP_CONTEXT * pdp) {
int32_t  ex, rval = 0;
int32_t n;
uint8_t context_id = MOD_CONTEXT_CONFIG_DEFAULT;

  if(context){
    if ((context->contextID == NULL)) {
      rval = MOD_DRIVER_ERROR_PARAMETER;
    }
    else if (context->contextID > MOD_CONTEXT_CONFIG_IDMAX && context->contextID < MOD_CONTEXT_CONFIG_IDMIN) {
      rval = MOD_DRIVER_ERROR_UNSUPPORTED;
    }
    else if (context->context_type) {
      if(!context->APN)
        rval = MOD_DRIVER_ERROR_PARAMETER;
      }
    }
  if(rval == 0){
    if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
          /* Mutex error */
          rval = MOD_DRIVER_ERROR;
    }
    else{ 
      rval = MOD_DRIVER_ERROR;

      ex = -1;
      if (context) {   
        context_id = context->contextID;

        rval = AT_Cmd_TCPIP_Context (context);
        if (rval == 0) {
          /* Wait until response arrives */
          ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT * 2);

          if (ex == 0) {
            /* Response arrived */
            ex = AT_Resp_Generic();
          }
        }
      }

      ex = AT_Cmd_Activate_PDP_Context(AT_CMODE_SET, context_id);

      if (ex == 0) {
          /* Wait until response arrives */
          ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT * 2);

          if (ex == 0) {
            /* Response arrived */
            ex = AT_Resp_Generic();
          }
        }
    
        if (ex != AT_RESP_OK) {

        if (ex == 1) {
          /* Connection timeout */
          rval = MOD_DRIVER_ERROR_TIMEOUT;
        }
        else if (ex == 2) {
          /* Wrong password */
          rval = MOD_DRIVER_ERROR;
        }
        else if (ex == 3) {
          /* Cannot find the target AP */
          /* Wrong SSID? Is AP down?   */
          rval = MOD_DRIVER_ERROR;
        }
        else {
          /* Connection failed, reason unknown */
          rval = MOD_DRIVER_ERROR;
        }
    }
      
      if(pdp){
        ex = AT_Cmd_Activate_PDP_Context(AT_CMODE_QUERY, context_id);
        if (ex == 0) {
            /* Wait until response arrives */
            ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT * 2);

            if (ex == 0) {
              /* Response arrived */
              ex = AT_Resp_Activate_PDP_Context(pdp);
              if(ex == 0 && pdp->type && pdp->state){
                /* Allocate socket (control block) */
                for (n = 0U; n < MOD_SOCKET_NUM; n++) {
                  if (PDPSocket[n].state == SOCKET_STATE_FREE) {
                    /* Found free socket, clear the socket structure */
                    memset (&PDPSocket[n], 0x00, sizeof(MOD_SOCKET));

                    /* Set initial state */
                    PDPSocket[n].state    = SOCKET_STATE_CREATED;

                    /* Set socket default parameters */
                    PDPSocket[n].type     = pdp->type;
                    PDPSocket[n].protocol = 0;
                    PDPSocket[n].server   = SOCKET_INVALID;
                    PDPSocket[n].backlog  = SOCKET_INVALID;
                    PDPSocket[n].conn_id  = pdp->id;
                    
                    PDPSocket[n].tout_rx  = 0U;
                    PDPSocket[n].tout_tx  = 0U;
                    PDPSocket[n].evflags_id = osEventFlagsNew (NULL);

                    /* Setup socket memory */
                    // BufInit (pCtrl->mempool_id, pCtrl->memmtx_id, &PDPSocket[n].mem);
                    break;
                  }
                }

                if (n == MOD_SOCKET_NUM) {
                  /* Not enough memory */
                  rval = ARM_SOCKET_ENOMEM;
                }
                else {
                  /* Return socket number */
                  rval = n;
                }
              }

            }
          }
      }

      if (osMutexRelease (pCtrl->mutex_id) != osOK) {
        /* Mutex error, override previous return value */
        rval = MOD_DRIVER_ERROR;
      }

    }
  }

  

  return (rval);

}

/**
  DeActivate interface.
  This command deactivates a specific context and closes all TCP/IP connections set up in this context.
Depending on the network, it may take at most 40 seconds to return OK or ERROR after AT+QIDEACT is
executed. The maximum timeout return time can be configured by
AT+QICFG="pdp/retry",<pdpmode>,<ratmode>[,<counts>,<retry_time>], before the response is
returned, other AT commands cannot be executed.

  \param[in]     context Interface settings.(can set MOD_ACTIVATE_DEFAULT active default 1 context)
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_TIMEOUT     : Timeout occurred
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported (security type, channel autodetect or WPS not supported)
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface, NULL config pointer or invalid configuration)
*/
static int32_t MOD_Deactivate (int32_t socket) {
  int32_t  ex, rval = 0;
  ex = -1;

  
  if ((socket < 0) || (socket >= MOD_PDPSOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  if(socket < 0) 
    PDPSocket[socket].conn_id = MOD_CONTEXT_CONFIG_DEFAULT;

  if (PDPSocket[socket].conn_id > MOD_CONTEXT_CONFIG_IDMAX && PDPSocket[socket].conn_id < MOD_CONTEXT_CONFIG_IDMIN) {
        rval = MOD_DRIVER_ERROR_UNSUPPORTED;
      }
  else if ( osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
    /* Mutex error */
    rval = MOD_DRIVER_ERROR;
    }  
  else{
    ex = AT_Cmd_Deactivate_PDP_Context( PDPSocket[socket].conn_id);
    if (ex == 0) {
        /* Wait until response arrives */
        ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT * 2);

        if (ex == 0) {
          /* Response arrived */
          ex = AT_Resp_Generic();
          if(ex == 0)
            PDPSocket[socket].state = SOCKET_STATE_FREE;
        }
      }
    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = MOD_DRIVER_ERROR;
    }
  }
  

  if (ex != AT_RESP_OK) {

    if (ex == 1) {
      /* Connection timeout */
      rval = MOD_DRIVER_ERROR_TIMEOUT;
    }
    else if (ex == 2) {
      /* Wrong password */
      rval = MOD_DRIVER_ERROR;
    }
    else if (ex == 3) {
      /* Cannot find the target AP */
      /* Wrong SSID? Is AP down?   */
      rval = MOD_DRIVER_ERROR;
    }
    else {
      /* Connection failed, reason unknown */
      rval = MOD_DRIVER_ERROR;
    }
  }
  return (rval);

}
/**
  \param[in]     option HTTP option, see also @MOD_HTTPOption_t enum struct
  \param[in]     data can be integer or char* data
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_TIMEOUT     : Timeout occurred
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported (security type, channel autodetect or WPS not supported)
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface, NULL config pointer or invalid configuration)
 

<contextID> Integer type. PDP context ID. Range: 1–7. Default: 1.
<request_header> Integer type. Disable or enable to customize HTTP(S) request header.
    0 Disable
    1 Enable
<response_header> Integer type. Disable or enable to output HTTP(S) response header.
    0 Disable
    1 Enable
<sslctxID> Integer type. SSL context ID used for HTTP(S). Range: 0–5. Default: 1. SSL
    parameters should be configured by AT+QSSLCFG. For details, See
    document [2].
<content_type> Integer type. Data type of HTTP(S) body.
    0 application/x-www-form-urlencoded
    1 text/plain
    2 application/octet-stream
    3 multipart/form-data
<auto_outrsp> Integer type. Disable or enable auto output of HTTP(S) response data. If auto
      output of HTTP(S) response data is enabled, then AT+QHTTPREAD and
      AT+QHTTPREADFILE will fail to execute.
      0 Disable
      1 Enable
<closedind> Integer type. Disable or enable report indication of closed HTTP(S) session.
    0 Disable
    1 Enable
<url_value> String type. The URL of HTTP(S).
<header_value> String type. HTTP(S) request header line/header field name, such as:
    "Content-type: text/plain" or "Content-type"
<user_pwd> String type. User name and password, the format is: "username:password"
<name> String type. The name value of form-data.
<file_name> String type. The filename value of form-data.
<content_type> String type. The content-type value of form-data.
<err> The error code of the operation. See Chapter 5.

*/
static int32_t MOD_HTTP_SetOption(MOD_HTTPOption_t option, void *data){
int32_t  ex, rval = 0;

   ex = -1;

    if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
        /* Mutex error */
        rval = MOD_DRIVER_ERROR;
    }
    else{
      ex = AT_Cmd_HTTP_Config(option, data);
      if (ex == 0) {
          /* Wait until response arrives */
          ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

          if (ex == 0) {
            /* Response arrived */
            ex = AT_Resp_Generic();
          }
          else{
            rval = AT_Resp_ErrCode (NULL);
            ex = AT_RESP_OK;
          }
        }
      if (osMutexRelease (pCtrl->mutex_id) != osOK) {
        /* Mutex error, override previous return value */
        rval = MOD_DRIVER_ERROR;
      }
    }

  if (ex != AT_RESP_OK) {

    if (ex == 1) {
      /* Connection timeout */
      rval = MOD_DRIVER_ERROR_TIMEOUT;
    }
    else if (ex == 2) {
      /* Wrong password */
      rval = MOD_DRIVER_ERROR;
    }
    else if (ex == 3) {
      /* Cannot find the target AP */
      /* Wrong SSID? Is AP down?   */
      rval = MOD_DRIVER_ERROR;
    }
    else {
      /* Connection failed, reason unknown */
      rval = MOD_DRIVER_ERROR;
    }
  }
  return (rval);
}


static int32_t MOD_HTTP_GetOption(MOD_HTTPOption_t option, char *resp, uint16_t length){
int32_t  ex, rval = 0;

   ex = -1;

    if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
        /* Mutex error */
        rval = MOD_DRIVER_ERROR;
    }
    else{
      ex = AT_Cmd_HTTP_Config(option, NULL);
      if (ex == 0) {
          /* Wait until response arrives */
          ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

          if (ex == 0) {
            /* Response arrived */
            ex = AT_Resp_HTTP_Config (resp, length); 
          }
          else{
            rval = AT_Resp_ErrCode (NULL);
            ex = AT_RESP_OK;
          }
        }
      if (osMutexRelease (pCtrl->mutex_id) != osOK) {
        /* Mutex error, override previous return value */
        rval = MOD_DRIVER_ERROR;
      }
    }

  if (ex != AT_RESP_OK) {

    if (ex == 1) {
      /* Connection timeout */
      rval = MOD_DRIVER_ERROR_TIMEOUT;
    }
    else if (ex == 2) {
      /* Wrong password */
      rval = MOD_DRIVER_ERROR;
    }
    else if (ex == 3) {
      /* Cannot find the target AP */
      /* Wrong SSID? Is AP down?   */
      rval = MOD_DRIVER_ERROR;
    }
    else {
      /* Connection failed, reason unknown */
      rval = MOD_DRIVER_ERROR;
    }
  }
  return (rval);
}

/**
  \param[in]     url 
  \param[in]     timeout 
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_TIMEOUT     : Timeout occurred
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported (security type, channel autodetect or WPS not supported)
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface, NULL config pointer or invalid configuration)
*/

static int32_t MOD_SetURL (char * url, uint32_t length, uint32_t timeout) {
  int32_t  ex, rval = 0;
  uint32_t cnt, len, num;

  ex = -1;
  len = length? length: strlen(url);

  if (len != 0) {

    if ( osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
        /* Mutex error */
        rval = MOD_DRIVER_ERROR;
    }
    else {
    /* 1. Message length is less than MAX (2048 bytes) */
    /* 2. For non-blocking socket, message can fit into buffer */
    /* 3. For blocking socket, message might need to be sent using multiple AT_Send_Data calls */
    
      /* Initiate send operation */
      ex = AT_Cmd_SendURL (AT_CMODE_SET, len,  timeout);
      /* Tx buffer full, wait until tx buffer available */
      ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

      if (ex != 0) {
        /* Serial driver error or device not accepting data */
        rval = ARM_SOCKET_ERROR;
      }
      else {
        /* Start sending actual data to device */
        /* Set number of bytes sent */
        num = 0U;

        while (num < len) {
          /* Determine amount of data to send */
          cnt = AT_Send_GetFree();

          if (cnt == 0U) {
            /* Tx buffer full, wait until tx buffer available */
            ex = Modem_Wait (MOD_WAIT_TX_DONE, MOD_RESP_TIMEOUT);

            if (ex == 0) {
              /* Data transfer completed */
              cnt = AT_Send_GetFree();
            }
          }

          if (cnt > (len - num)) {
            cnt = (len - num);
          }

          osEventFlagsClear (pCtrl->evflags_id, MOD_WAIT_TX_DONE);

          num += AT_Send_Data ((uint8_t *)&url[num], cnt);
        }

        /* Data sent, wait for SEND OK or SEND FAIL responses */
        ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

        if (ex == 0) {
          /* Check response */
          ex = AT_Resp_Generic();

          if (ex == 0) {
            /* Packet sent, return number of bytes sent */
            rval = (int32_t)num;
          }
          else {
            /* Should not happend */
            rval = MOD_DRIVER_ERROR;
          }
        }
        else{
            rval = AT_Resp_ErrCode (NULL);
            ex = AT_RESP_OK;
          }
      }
    }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
        /* Mutex error, override previous return value */
        rval = MOD_DRIVER_ERROR;
      }
  }

  if (ex != AT_RESP_OK) {

    if (ex == 1) {
      /* Connection timeout */
      rval = MOD_DRIVER_ERROR_TIMEOUT;
    }
    else if (ex == 2) {
      /* Wrong password */
      rval = MOD_DRIVER_ERROR;
    }
    else if (ex == 3) {
      /* Cannot find the target AP */
      /* Wrong SSID? Is AP down?   */
      rval = MOD_DRIVER_ERROR;
    }
    else {
      /* Connection failed, reason unknown */
      rval = MOD_DRIVER_ERROR;
    }
  }
  return (rval);

}


/**
  \brief HTTP GET/POST request handler
  \param[in]     socket activated PDP socket ID 
  \param[in]     httpd  MOD_HTTP_t: HTTP request type def
  \note
    -MOD_HTTP_t:
      - method: Request method
        - MOD_HTTP_POST
        - MOD_HTTP_GET

      - url: type uint8_t *; Request URL
      - url_length: type uint32_t(optional if URL is an ascii string); 

      - data_callback: type function pointer; optional; 
        - Request data callback that used to send fragmented HTTP content.
        -   data: out; type char **;  data buffer
        -   counter: input; loop counter(start from zero)
        -   remain_size: input; The remaining size of the sent data
        -   retval: Length of written data 

      - data: type uint8_t *; pointer to data buffer (HTTP content)

      - data_length: (optional) length of the request content.
        - required if needed to send fragmented data;
        - optional if data pointer is not a NULL value.

      - header: type MOD_Header_t
        - HTTP request header type.
        - MOD_Header_t: 
          - fields:  pointer to HTTP_HeaderField array; Header fields
            - HTTP_HeaderField: header fields
              - name: (char *)
              - value: (char *)
          - nfield: number of header fields
          - authorization: authorization token
          - buffer: temporary static buffer to process header fields 
        
      
      - response: (optional); type uint8_t *; 
        - pointer to response buffer (HTTP response) if response_callback is null
          else pointer to data buffer used in response_callback
          (with size of the response_length * 2 in static allocation mode)

      - response_callback: type function pointer; optional; 
        - HTTP Response callback that used to receive fragmented HTTP content.
        - data: IN; type uint8_t *; pointer to response buffer.
        - counter: input; loop counter(start from zero)
        - remain_size: type uint32_t:  The remaining size of the received data
        - size: size of current data 

      - uint32_t response_length: optional; 
        -  if the response buffer is not null, the required response length must be taken)

      - response_header: 
        - MOD_HTTP_ENABLE
        - MOD_HTTP_NORESP

      - uint32_t timeout:
        - request timeout(second)

      - uint32_t resptime:
        - response timeout(second)

  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_TIMEOUT     : Timeout occurred
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported (security type, channel autodetect or WPS not supported)
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface, NULL config pointer or invalid configuration)
*/

static int32_t MOD_HTTP (int32_t socket, MOD_HTTP_t * httpd){

  int32_t  ex = 0, rval = 0;
	uint32_t tmp = 0, i = 0, j;
  MOD_SOCKET *sock;
  uint8_t * data;
  char *seprator = "\r\n";

  if(!httpd){
    rval = ARM_SOCKET_ESOCK;
  }
  else if ((socket < 0) || (socket >= MOD_PDPSOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else{
    sock = &PDPSocket[socket];
    if ((!sock->type) ||  (!sock->conn_id)) {
      /* Accept is not supported for UDP sockets */
      rval = ARM_SOCKET_ENOTSUP;
      ex = 0;
    }
    else if(sock->state != SOCKET_STATE_CREATED && sock->state != SOCKET_STATE_BOUND){
      rval = ARM_SOCKET_ENOTSUP;
      ex = 0;
    }
    else {
      ex = MOD_HTTP_SetOption(HTTP_OPTION_CONTEXT_ID, (void *)sock->conn_id);
      
			if(httpd->data && !httpd->data_length)
          httpd->data_length = strlen((const char *)httpd->data);
			
      if(httpd->header){
        HTTP_HeaderTypeDef header;

        ex = MOD_HTTP_SetOption(HTTP_OPTION_REQUESTHEADER, (void *)HTTP_SETOPTION_ENABLE);

        header.fields = httpd->header->fields;
        header.nfield = httpd->header->nfield;
        header.url = (char *)httpd->url;
        header.seprator = seprator;
        header.method = httpd->method;
        header.content_length = httpd->data_length;
        tmp = HTTPHeader(httpd->header->buffer, &header);
        }
        
        
      if(httpd->response_header)
        ex = MOD_HTTP_SetOption(HTTP_OPTION_RESPONSEHEADER, (void *)HTTP_SETOPTION_ENABLE);
      

      if(httpd->url[4] == 's'){ //enable SSL config TODO:
        uint32_t SSL_context_id = 1;
        MOD_HTTP_SetOption(HTTP_OPTION_SSLCTXID, (void *)SSL_context_id);
        MOD_SSL_SetOption(SSL_CONFIG_VERSION, SSL_context_id, (void *)SSL_PARAM_VERSION_ALL);
        MOD_SSL_SetOption(SSL_CONFIG_CIPHER_SUITE, SSL_context_id, (void *)SSL_PARAM_CIPHER_SUPPORT_ALL);
        MOD_SSL_SetOption(SSL_CONFIG_SECLEVEL, SSL_context_id, (void *)SSL_PARAM_SECLEVEL_FREE);
        // MOD_SSL_SetOption(SSL_CONFIG_CACERT, SSL_context_id, (void *)"UFS:cacert.pem");
        // MOD_SSL_SetOption(SSL_CONFIG_CLIENTCERT, SSL_context_id, (void *)"UFS:clientcert.pem");
        // MOD_SSL_SetOption(SSL_CONFIG_CLIENTKEY, SSL_context_id, (void *)"UFS:clientkey.pem");

      }


      if(httpd->url){
        // char *path = strchr(strstr((char *)httpd->url, "//") + 2, '/');
        // char tmp = *path;
        // *path = 0;
        ex = MOD_SetURL((char *)httpd->url, httpd->url_length, httpd->timeout);
        // *path = tmp;
      }
      
      if(ex < 0){
        rval = MOD_DRIVER_ERROR;
      } 
      // else if(AT_Cmd_Activate_PDP_Context(AT_CMODE_SET, sock->conn_id) != 0){
      //   rval = MOD_DRIVER_ERROR;
      // }
      // else if(Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT * 30) != 0){
      //   rval = MOD_DRIVER_ERROR;
      // }
      else{

        if(httpd->method == MOD_HTTP_POST)
          ex = MOD_HTTP_POST_Open(socket, httpd->data_length + tmp, httpd->timeout, httpd->resptime);
        else    
          ex = MOD_HTTP_GET_Open(socket, httpd->data_length + tmp, httpd->timeout, httpd->resptime);
 
        if(ex >= 0){
          
					if(httpd->header)
						MOD_HTTP_Send(socket, (uint8_t *)httpd->header->buffer, tmp);

					if(httpd->data){
						MOD_HTTP_Send(socket, httpd->data, httpd->data_length);
					}
					else if(httpd->data_callback){
						
						for(tmp= 0; tmp < httpd->data_length; tmp += i){
							j = httpd->data_length - tmp;
							i = httpd->data_callback(&data, tmp, j);
							if(i > j) i = j;
							MOD_HTTP_Send(socket, data, i);
						}
					}


					ex = MOD_HTTP_End(socket, httpd->response, httpd->response_callback ,httpd->response_length, httpd->resptime);

          sock->state = SOCKET_STATE_BOUND;
			  }
      }
    }
  }

  // osDelay(10000);
  
  if (ex < 0) {

    if (ex == -1) {
      /* Connection timeout */
      rval = MOD_DRIVER_ERROR_TIMEOUT;
    }
    else if (ex == -2) {
      /* Wrong password */
      rval = MOD_DRIVER_ERROR_SPECIFIC;
    }
    else {
      /* Connection failed, reason unknown */
      rval = MOD_DRIVER_ERROR;
    }
   
  }
	else if(ex > 0)
		rval = ex;
	
  return (rval);

}

/**
 * send post request
 * 
  \param[in]     data_length 
  \param[in]     timeout 
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR_SPECIFIC             : Operation failed
                   - \ref MOD_DRIVER_ERROR_TIMEOUT     : Timeout occurred
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported (security type, channel autodetect or WPS not supported)
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface, NULL config pointer or invalid configuration)
*/

static int32_t MOD_HTTP_POST_Open (int32_t socket, uint32_t data_length, uint32_t timeout, uint32_t resptime) {
  int32_t  ex, rval = 0;
  MOD_SOCKET *sock;

  ex = -1;

  if (data_length == 0) {
      rval = MOD_DRIVER_ERROR_PARAMETER;
  }
  else if ((socket < 0) || (socket >= MOD_PDPSOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else{
    sock = &PDPSocket[socket];
      
    if ((!sock->type) ||  (!sock->conn_id)) {
      /* Accept is not supported for UDP sockets */
      rval = ARM_SOCKET_ENOTSUP;
      ex = 0;
    }
    else if(sock->state != SOCKET_STATE_CREATED && sock->state != SOCKET_STATE_BOUND){
      rval = ARM_SOCKET_ENOTSUP;
      ex = 0;
    }
    else {
      

      if ( osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
          /* Mutex error */
          rval = MOD_DRIVER_ERROR;
      }      
      else{
        
        ex = AT_Cmd_SendPOST (data_length,  timeout, resptime);
        /* Tx buffer full, wait until tx buffer available */
        ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, timeout * 1000U);
        
        sock->state = SOCKET_STATE_CONNECTED;
        /* Initiate send operation */

        if (osMutexRelease (pCtrl->mutex_id) != osOK) {
          /* Mutex error, override previous return value */
          rval = MOD_DRIVER_ERROR;
        }
      } 
    }
  }

  // osDelay(10000);
  
  if (ex != AT_RESP_OK) {

    if (ex == -1) {
      /* Connection timeout */
      rval = MOD_DRIVER_ERROR_TIMEOUT;
    }
    else if (ex == -2) {
      /* Wrong password */
      rval = MOD_DRIVER_ERROR_SPECIFIC;
    }
    else {
      /* Connection failed, reason unknown */
      rval = MOD_DRIVER_ERROR;
    }
   
  }
  return (rval);

}


/**
 * send GET request
 * 
  \param[in]     data_length 
  \param[in]     timeout 
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR_SPECIFIC             : Operation failed
                   - \ref MOD_DRIVER_ERROR_TIMEOUT     : Timeout occurred
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported (security type, channel autodetect or WPS not supported)
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface, NULL config pointer or invalid configuration)
*/

static int32_t MOD_HTTP_GET_Open (int32_t socket, uint32_t data_length, uint32_t timeout, uint32_t resptime) {
  int32_t  ex, rval = 0;
  MOD_SOCKET *sock;

  ex = -1;

  if ((socket < 0) || (socket >= MOD_PDPSOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else{
    sock = &PDPSocket[socket];
      
    if ((!sock->type) ||  (!sock->conn_id)) {
      /* Accept is not supported for UDP sockets */
      rval = ARM_SOCKET_ENOTSUP;
      ex = 0;
    }
    else if(sock->state != SOCKET_STATE_CREATED && sock->state != SOCKET_STATE_BOUND){
      rval = ARM_SOCKET_ENOTSUP;
      ex = 0;
    }
    else {
      

      if ( osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
          /* Mutex error */
          rval = MOD_DRIVER_ERROR;
      }      
      else{
        
        /* Initiate send operation */
        ex = AT_Cmd_SendGET (resptime, data_length,  timeout);
        /* Tx buffer full, wait until tx buffer available */
        // ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, timeout * 1000U);
        sock->state = SOCKET_STATE_CONNECTED;

        if (osMutexRelease (pCtrl->mutex_id) != osOK) {
          /* Mutex error, override previous return value */
          rval = MOD_DRIVER_ERROR;
        }
      } 
    }
  }

  // osDelay(10000);
  
  if (ex != AT_RESP_OK) {

    if (ex == -1) {
      /* Connection timeout */
      rval = MOD_DRIVER_ERROR_TIMEOUT;
    }
    else if (ex == -2) {
      /* Wrong password */
      rval = MOD_DRIVER_ERROR_SPECIFIC;
    }
    else {
      /* Connection failed, reason unknown */
      rval = MOD_DRIVER_ERROR;
    }
   
  }
  return (rval);

}

/**
 * send post request data
 * 
  \param[in]     url 
  \param[in]     timeout 
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_TIMEOUT     : Timeout occurred
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported (security type, channel autodetect or WPS not supported)
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface, NULL config pointer or invalid configuration)
*/

static int32_t MOD_HTTP_Send (int32_t socket, uint8_t  *data, uint32_t len) {
  int32_t  ex, rval = 0;
  uint32_t cnt, num;
  MOD_SOCKET *sock;

  ex = -1;

  if (!len) {
    rval = MOD_DRIVER_ERROR_PARAMETER;
  }
  else if ((socket < 0) || (socket >= MOD_PDPSOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  if ( osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
      /* Mutex error */
      rval = MOD_DRIVER_ERROR;
  } 
  else{
    sock = &PDPSocket[socket];
  /* 1. Message length is less than MAX (2048 bytes) */
  /* 2. For non-blocking socket, message can fit into buffer */
  /* 3. For blocking socket, message might need to be sent using multiple AT_Send_Data calls */
  if(sock->state != SOCKET_STATE_CONNECTED){
    rval = ARM_SOCKET_ERROR;
  }
  else{
    /* Start sending actual data to device */
    /* Set number of bytes sent */
    num = 0U;

    while (num < len) {
      /* Determine amount of data to send */
      cnt = AT_Send_GetFree();

      if (cnt == 0U) {
        /* Tx buffer full, wait until tx buffer available */
        ex = Modem_Wait (MOD_WAIT_TX_DONE, MOD_RESP_TIMEOUT);

        if (ex == 0) {
          /* Data transfer completed */
          cnt = AT_Send_GetFree();
        }
        else {
          /* Device internal error */
          rval = ARM_SOCKET_ERROR;
          break;
        }
      }

      if (cnt > (len - num)) {
        cnt = (len - num);
      }

      osEventFlagsClear (pCtrl->evflags_id, MOD_WAIT_TX_DONE);

      num += AT_Send_Data ((uint8_t *)&data[num], cnt);
    }
    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = MOD_DRIVER_ERROR;
    }
  }
}    

if (ex != AT_RESP_OK) {

  if (ex == -1) {
    /* Connection timeout */
    rval = MOD_DRIVER_ERROR_TIMEOUT;
  }
  else if (ex == -2) {
    /* Wrong password */
    rval = MOD_DRIVER_ERROR_SPECIFIC;
  }
  else {
    /* Connection failed, reason unknown */
    rval = MOD_DRIVER_ERROR;
  }
  
}
return (rval);

}
/**
 * end of the HTTP request(recieve data)
 * 
  \param[in]     timeout 
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_TIMEOUT     : Timeout occurred
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported (security type, channel autodetect or WPS not supported)
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface, NULL config pointer or invalid configuration)
*/

static int32_t MOD_HTTP_End ( int32_t socket, 
                              uint8_t * buf, 
                              HTTP_ResponseCallback_t response_callback, 
                              uint32_t len, 
                              uint32_t timeout) {

  int32_t  ex, rval = 0, httpstate = 0, val;
  uint8_t dynamic_alloc = 0;
  MOD_SOCKET *sock;
  ex = -1;  
  
  Modem_ThreadKick();

  if ((socket < 0) || (socket >= MOD_PDPSOCKET_NUM)) {
    /* Invalid socket identification number */
    rval = ARM_SOCKET_ESOCK;
  }
  else if ( osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
          /* Mutex error */
      rval = MOD_DRIVER_ERROR;
  }      
  else{
    sock = &PDPSocket[socket];
    sock->rx_len = 0;
    sock->mem.mp_id = 0;
    sock->response_callback = NULL;
    sock->response_callback_size = NULL;
    /* Data sent, wait for SEND OK or SEND FAIL responses OK*/
    ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);
    
    if(!len && buf)
      rval = MOD_DRIVER_ERROR_PARAMETER;

    else if ((!sock->type) ||  (!sock->conn_id)) {
      /* Accept is not supported for UDP sockets */
      rval = ARM_SOCKET_ENOTSUP;
      ex = 0;
    }
    else if(ex == 0 && Modem_Wait (MOD_WAIT_HTTP_RESPONSE, timeout * 1000UL ) == 0){ /*+Qread*/
        // osDelay(100);
        //Read HTTP error code 
        AT_Resp_HTTPErrCode ((uint32_t *)&rval, (uint32_t *)&httpstate, &sock->rx_len);
				
				if(rval){
					
				}
				else if(sock->rx_len < 1 /*HTTP1.0*/){
					sock->rx_len = len;
				}	
        else{  //successfully sended data

            if(sock->state != SOCKET_STATE_CONNECTED){
              rval = ARM_SOCKET_ERROR;
              ex = AT_RESP_OK;
            }
            else{

              // rx_len is size of rx remain bytes
              // check rx_len size
							if(len && (response_callback == NULL))
								sock->rx_len = ((len > sock->rx_len)?  sock->rx_len: len);
                
              sock->response_callback_size = len;
              sock->response_callback = response_callback;
              sock->current.count = 0;
              sock->current.mem = 0;
              sock->current.mem_size = 0;
              sock->current.remain_size = 0;
              
              if(buf){
                sock->rx_mem[0] = (void *)buf;
                sock->rx_mem[1] = (void *)buf + len;
							}
              else if(response_callback){
                sock->rx_mem[0] = (void *)malloc(SOCKET_BUFFER_BLOCK_SIZE);
                sock->rx_mem[1] = (void *)malloc(SOCKET_BUFFER_BLOCK_SIZE);
                dynamic_alloc = 1;
                sock->response_callback_size = SOCKET_BUFFER_BLOCK_SIZE;
              }
              // tout_rx rx size in byte
              sock->tout_rx = 0;
              sock->current.count = 0;

              if(AT_Cmd_QHTTPREAD(timeout) != 0){
                ex = -1;
              }
              else if(Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT) != 0/*CONNECT*/){
                ex = -1;
              }
              else if (ex == 0){

                if(sock->response_callback){
                  osEventFlagsSet (sock->evflags_id, SOCK_WAIT_HTTP_RESP_PARTIAL_END);
                }

                //ex = AT_RespData_HTTP(buf, len);
                while(1){

                  val = osEventFlagsWait (sock->evflags_id, 
                                          SOCK_WAIT_HTTP_RESP_PARTIAL | SOCK_WAIT_HTTP_RESP_COMPLETE, 
                                          osFlagsWaitAny, 
                                          osWaitForever);

									if(val & osFlagsError/*Flag is NULL*/){
										continue;
									}									
                  
                  if(sock->response_callback){
                    sock->response_callback(sock->current.mem,
                                            sock->current.count/*counter*/, 
                                            sock->current.remain_size,
                                            /*current data size*/sock->current.mem_size);
                  }

                  if(val == SOCK_WAIT_HTTP_RESP_COMPLETE){
										break;
                  }

                  osEventFlagsSet (sock->evflags_id, SOCK_WAIT_HTTP_RESP_PARTIAL_END);
                }
                
              
                if (ex == 0) {
                  ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT); // OK
                }
                else{
                  osEventFlagsSet (sock->evflags_id, SOCK_WAIT_HTTP_RESP_FAIL);
                }

                ex = Modem_Wait (MOD_WAIT_HTTP_RESPONSE, MOD_RESP_TIMEOUT) ; // wait for +QHTTPRREAD

                if(ex == 0)
                  ex = AT_Resp_HTTPErrCode ((uint32_t *)&rval, NULL, NULL);

                  
              }
              sock->state = SOCKET_STATE_BOUND;
          }

          
        }
        
        
      }

    if (osMutexRelease (pCtrl->mutex_id) != osOK) {
        /* Mutex error, override previous return value */
        rval = MOD_DRIVER_ERROR;
      }
  }
  
  if(dynamic_alloc)
    free(sock->mem.mp_id);

  if(httpstate && rval == 0 ){
    rval = httpstate;
  }
  else if (ex != AT_RESP_OK) {

    if (ex == -1) {
      /* Connection timeout */
      rval = MOD_DRIVER_ERROR_TIMEOUT;
    }
    else if (ex == -2) {
      /* Wrong password */
      rval = MOD_DRIVER_ERROR_SPECIFIC;
    }
    else {
      /* Connection failed, reason unknown */
      rval = MOD_DRIVER_ERROR;
    }
   
  }
  return (rval);

}



static int32_t MOD_SSL_SetOption(SSL_Config_t option, uint8_t ssl_context_id, void * data){
int32_t  ex, rval = 0;

   ex = -1;

    if (osMutexAcquire (pCtrl->mutex_id, osWaitForever) != osOK) {
        /* Mutex error */
        rval = MOD_DRIVER_ERROR;
    }
    else{
      ex = AT_Cmd_SSL_Config (option, ssl_context_id, data);
      if (ex == 0) {
          /* Wait until response arrives */
          ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

          if (ex == 0) {
            /* Response arrived */
            ex = AT_Resp_Generic();
          }
          else{
            rval = AT_Resp_ErrCode (NULL);
            ex = AT_RESP_OK;
          }
        }
      if (osMutexRelease (pCtrl->mutex_id) != osOK) {
        /* Mutex error, override previous return value */
        rval = MOD_DRIVER_ERROR;
      }
    }

  if (ex != AT_RESP_OK) {

    if (ex == 1) {
      /* Connection timeout */
      rval = MOD_DRIVER_ERROR_TIMEOUT;
    }
    else if (ex == 2) {
      /* Wrong password */
      rval = MOD_DRIVER_ERROR;
    }
    else if (ex == 3) {
      /* Cannot find the target AP */
      /* Wrong SSID? Is AP down?   */
      rval = MOD_DRIVER_ERROR;
    }
    else {
      /* Connection failed, reason unknown */
      rval = MOD_DRIVER_ERROR;
    }
  }
  return (rval);
}


/**
 * send post request
 * 
  \param[in]     url 
  \param[in]     timeout 
  \return        execution status
                   - \ref MOD_DRIVER_OK                : Operation successful
                   - \ref MOD_DRIVER_ERROR             : Operation failed
                   - \ref MOD_DRIVER_ERROR_TIMEOUT     : Timeout occurred
                   - \ref MOD_DRIVER_ERROR_UNSUPPORTED : Operation not supported (security type, channel autodetect or WPS not supported)
                   - \ref MOD_DRIVER_ERROR_PARAMETER   : Parameter error (invalid interface, NULL config pointer or invalid configuration)
*/

static int32_t MOD_Release (void) {
  int rval = 0;

  if (osMutexRelease (pCtrl->mutex_id) != osOK) {
      /* Mutex error, override previous return value */
      rval = MOD_DRIVER_ERROR;
    }

  return (rval);

}

/* Exported MOD_DRIVER# */
MOD_DRIVER MOD_DRIVER_(MOD_DRIVER_NUMBER) = {
  MOD_GetVersion,
  MOD_GetCapabilities,
  MOD_Initialize,
  MOD_Uninitialize,
  MOD_PowerControl,
  MOD_Activate,
  MOD_Deactivate,
  MOD_Context,
  MOD_HTTP_SetOption,
  MOD_HTTP_GetOption,
  MOD_SetURL,
  MOD_HTTP,
  MOD_Release,
  MOD_SSL_SetOption,
};

static int32_t ResetModule (void) {
  int32_t rval, ex;

  ex = AT_Cmd_Reset();

  if (ex == 0) {
    /* Wait until response arrives */
    ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, 5000);

    if (ex == 0) {
      /* Response received */
      ex = AT_Resp_Generic();
      
      if (ex == AT_RESP_OK) {
        /* Wait until response arrives */
        /* Reset generates second response */
        ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, 5000);

        if (ex == 0) {
          /* Response received */
          ex = AT_Resp_Generic();
          
          if (ex == AT_RESP_READY) {
            /* Successfully reset */
            ex = 0;
          }
        }
      }
    }
  }

  if (ex == 0) {
    rval = MOD_DRIVER_OK;
  } else {
    rval = MOD_DRIVER_ERROR;
  }

  return (rval);
}

static int32_t SetupCommunication (void) {
  int32_t rval, ex;
  int32_t state;
  uint32_t k;
  uint32_t stop_par_flowc;
  AT_PARSER_COM_SERIAL info;
  const uint32_t br[] = {MOD_SERIAL_BAUDRATE, 115200,
                                               230400,  460800,  921600,
                                              1000000, 1500000, 2000000,
                                                57600,   38400,   19200, 9600};
  k     =  0;
  state =  1;

  ex = AT_Parser_GetSerialCfg (&info);

  if (ex == 0) {
    /* Set interface mode */
    info.databits     = 8U;
    info.stopbits     = 1U;
    info.parity       = 0U;
    info.flow_control = 0U;
  }

  while (state < 3) {

    switch (state) {
      case 0:
      case 2:
        /* Select baudrate and reconfigure serial interface */
        info.baudrate = br[k];

        ex = AT_Parser_SetSerialCfg (&info);
        // return MOD_DRIVER_OK;


        if (ex != 0) {
          /* Error */
          break;
        }

        /* Send test command (check if we will get a response) */
        ex = AT_Cmd_TestAT();
        break;

      case 1:
        /* Reconfigure UART */
        // stop_par_flowc = (info.stopbits << 4) |
        //                  (info.parity   << 2) |
        //                   info.flow_control   ;

        // ex = AT_Cmd_ConfigUART (AT_CMODE_SET, br[k], info.databits, stop_par_flowc);
        ex = AT_Cmd_ConfigUARTRate (AT_CMODE_SET, br[k]);
        //ex = 0;
        break;

      default:
        ex = -1;
        break;
    }

    if (ex != 0) {
      /* Error, exit loop */
      state = 4;
    }
    else {
      /* Wait until response arrives */
      ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, 200);

      if (ex != 0) {
        /* No response, reconfigure and try again */
        AT_Parser_Reset();

        /* Try again */
        state = 0;

        /* Change baudrate */
        k++;

        if (k == (sizeof(br)/sizeof(br[0]))) {
          /* Tried all baud rates, no response, exit loop */
          state = 4;
        }
      }
      else {
        /* Response received */
        ex = AT_Resp_Generic();

        if ((ex == AT_RESP_OK) || (ex == AT_RESP_ERROR)) {
          if (state == 0) {
            /* Communication established */
            if (k != 0) {
              /* Select configured baud rate */
              k = 0;
            }
            else {
              /* Already using config baud rate, skip reconfiguration */
              state = 2;
            }
          }
        }

        /* Next step */
        state++;
      }
    }
  }

  if (ex == 0) {
    rval = MOD_DRIVER_OK;
  } else {
    rval = MOD_DRIVER_ERROR;
  }

  return (rval);
}

static int32_t IsUnspecifiedIP (const uint8_t ip[]) {
  int32_t rval;

  if ((ip[0] == 0U) && (ip[1] == 0U) && (ip[2] == 0U) && (ip[3] == 0U)) {
    /* Unspecified IP address */
    rval = 1U;
  } else {
    rval = 0U;
  }

  return (rval);
}

static int32_t GetCurrentMAC (uint32_t interface, uint8_t mac[]) {
  int32_t ex;

  if (interface == MOD_INTERFACE_STATION) {
    ex = AT_Cmd_StationMAC (AT_CMODE_QUERY, NULL);
  } else {
    ex = AT_Cmd_AccessPointMAC (AT_CMODE_QUERY, NULL);
  }

  if (ex == 0) {
    /* Wait until response arrives */
    ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

    if (ex == 0) {
      /* Check response */
      if (interface == MOD_INTERFACE_STATION) {
        ex = AT_Resp_StationMAC (mac);
      } else {
        ex = AT_Resp_AccessPointMAC (mac);
      }
    }
  }

  return (ex);
}

static int32_t GetCurrentIpAddr (uint32_t interface, uint8_t ip[], uint8_t gw[], uint8_t mask[]) {
  int32_t ex;
  uint8_t *p;

  if (interface == MOD_INTERFACE_STATION) {
    ex = AT_Cmd_StationIP (AT_CMODE_QUERY, NULL, NULL, NULL);
  } else {
    ex = AT_Cmd_AccessPointIP (AT_CMODE_QUERY, NULL, NULL, NULL);
  }

  if (ex == 0) {
    /* Wait until response arrives */
    ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

    if (ex == 0) {
      /* Check response */
      p = ip;

      do {
        if (interface == MOD_INTERFACE_STATION) {
          ex = AT_Resp_StationIP (p);
        } else {
          ex = AT_Resp_AccessPointIP (p);
        }

        /* Set appropriate IP array */
        if (p == ip) { p = gw;   }
        else         { p = mask; }
      }
      while (ex == 1);
    }
  }
  return (ex);
}


static int32_t GetCurrentDnsAddr (uint32_t interface, uint8_t dns0[], uint8_t dns1[]) {
  int32_t ex;
  uint8_t *p;
  (void)interface;  /* The same DNS is used for both interfaces */

  ex = AT_Cmd_DNS (AT_CMODE_QUERY, 0, NULL, NULL);

  if (ex == 0) {
    /* Wait until response arrives */
    ex = Modem_Wait (MOD_WAIT_RESP_GENERIC, MOD_RESP_TIMEOUT);

    if (ex == 0) {
      /* Check response */
      p = dns0;

      #if (AT_VARIANT == AT_VARIANT_EG915U) && (AT_VERSION >= AT_VERSION_2_0_0_0)
      ex = AT_Resp_DNS (NULL, p, dns1);
      #else
      do {
        ex = AT_Resp_DNS (p);

        /* Set appropriate IP array */
        if (p == dns0) { p = dns1; }
        else           { p = NULL; }
      }
      while (ex == 1);
      #endif
    }
  }
  return (ex);
}

/*
  Extract option value from specified address and with given length

  opt_val: address of option value argument
  opt_len: length of option value argument in bytes (max 4 bytes)
  return: extracted value as uint32_t
*/
static uint32_t GetOpt (const void *opt_val, uint32_t opt_len) {
  uint32_t val;

  #ifdef __UNALIGNED_UINT32_READ
    (void)opt_len;

    val = __UNALIGNED_UINT32_READ(opt_val);
  #else
    uint32_t i, pos;
    const uint8_t *p = opt_val;

    pos = 0U;
    val = 0U;

    for (i = 0U; i < opt_len; i++) {
      val |= (p[i] & 0xFFU) << pos;

      pos += 8U;
    }
  #endif

  return (val);
}

/*
  Assign option value to variable at specified address

  opt_val: address of option value variable
  val:     option value
  opt_len: length of option value in bytes (max 4 bytes)
  return: length of assigned value in bytes
*/
static uint32_t SetOpt (void *opt_val, uint32_t val, uint32_t opt_len) {
  uint32_t len;

  #ifdef __UNALIGNED_UINT32_WRITE
    /* Write val into opt_val */
    __UNALIGNED_UINT32_WRITE (opt_val, val);

    len = opt_len;
  #else
    uint32_t pos;
    uint8_t *p = opt_val;

    pos = 0U;

    for (len = 0U; len < opt_len; len++) {
      p[len] = (val >> pos) & 0xFFU;

      pos += 8U;
    }
  #endif

  return (len);
}

/* Return free connection id */
static uint32_t ConnId_Alloc (void) {
  uint32_t bit;

  for (bit = 0; bit < CONN_ID_INVALID; bit++) {
    if ((pCtrl->conn_id & (1U << bit)) == 0) {
      break;
    }
  }

  return (bit);
}

/* Clear flag, connection id is free to use */
static void ConnId_Free  (uint32_t conn_id) {
  pCtrl->conn_id &= ~(1U << conn_id);
}

/* Set flag, connection id is taken */
static void ConnId_Accept (uint32_t conn_id) {
  pCtrl->conn_id |= (1U << conn_id);
}




