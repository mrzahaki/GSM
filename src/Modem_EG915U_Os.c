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
#include "Modem_EG915U_Os.h"

#define THREAD_CC_ATTR     __attribute__((section(".bss.os.thread.cb")))
#define MEMPOOL_CC_ATTR    __attribute__((section(".bss.os.mempool.cb")))
#define EVENTFLAGS_CC_ATTR __attribute__((section(".bss.os.evflags.cb")))
#define MUTEX_CC_ATTR      __attribute__((section(".bss.os.mutex.cb")))

/* --------------------------------------------------------------------------*/

#define MOD_THREAD_STACK_ARR_SIZE   MOD_THREAD_STACK_SIZE

/* Stack memory */
static uint8_t Modem_ThreadCb[OS_THREAD_CB_SIZE]                __ALIGNED(4) THREAD_CC_ATTR;
static uint8_t Modem_ThreadStackArr[MOD_THREAD_STACK_ARR_SIZE] __ALIGNED(8);

/* Modem thread */
const osThreadAttr_t Modem_Thread_Attr = {
  .name       = "Modem Thread",
  .attr_bits  = osThreadDetached,
  .cb_mem     = Modem_ThreadCb,
  .cb_size    = sizeof(Modem_ThreadCb),
  .stack_mem  = Modem_ThreadStackArr,
  .stack_size = sizeof (Modem_ThreadStackArr),
  .priority   = MOD_THREAD_PRIORITY,
  .tz_module  = 0,
};

/* --------------------------------------------------------------------------*/

/* Memory Pool array size */
#define SOCKET_MEMPOOL_ARR_SIZE    OS_MEMPOOL_MEM_SIZE(SOCKET_BUFFER_BLOCK_COUNT, SOCKET_BUFFER_BLOCK_SIZE)

/* Memory Pool control block and memory space */
static uint8_t Socket_MemPoolCb[OS_MEMPOOL_CB_SIZE]       __ALIGNED(4) MEMPOOL_CC_ATTR;
static uint8_t Socket_MemPoolArr[SOCKET_MEMPOOL_ARR_SIZE] __ALIGNED(4);

/* Memory pool for socket data storage */
const osMemoryPoolAttr_t Socket_MemPool_Attr = {
  .name      = "Modem Socket",
  .attr_bits = 0U,
  .cb_mem    = Socket_MemPoolCb,
  .cb_size   = sizeof(Socket_MemPoolCb),
  .mp_mem    = Socket_MemPoolArr,
  .mp_size   = sizeof(Socket_MemPoolArr)
};

/* --------------------------------------------------------------------------*/

#define MOD_MUTEX_ATTRIBUTES  osMutexPrioInherit

static uint8_t Socket_MutexCb[OS_MUTEX_CB_SIZE] __ALIGNED(4) MUTEX_CC_ATTR;

const osMutexAttr_t Socket_Mutex_Attr = {
  .name      = "Modem Socket",
  .attr_bits = MOD_MUTEX_ATTRIBUTES,
  .cb_mem    = Socket_MutexCb,
  .cb_size   = sizeof(Socket_MutexCb)
};

/* --------------------------------------------------------------------------*/

#define ATPARSER_MEMPOOL_ARR_SIZE     OS_MEMPOOL_MEM_SIZE(PARSER_BUFFER_BLOCK_COUNT, PARSER_BUFFER_BLOCK_SIZE)

/* Memory Pool control block and memory space */
static uint8_t AT_Parser_MemPoolCb[OS_MEMPOOL_CB_SIZE]         __ALIGNED(4) MEMPOOL_CC_ATTR;
static uint8_t AT_Parser_MemPoolArr[ATPARSER_MEMPOOL_ARR_SIZE] __ALIGNED(4) ;

/* Memory Pool for AT command parser */
const osMemoryPoolAttr_t AT_Parser_MemPool_Attr = {
  .name      = "Modem Parser",
  .attr_bits = 0U,
  .cb_mem    = AT_Parser_MemPoolCb,
  .cb_size   = sizeof(AT_Parser_MemPoolCb),
  .mp_mem    = AT_Parser_MemPoolArr,
  .mp_size   = sizeof(AT_Parser_MemPoolArr),
};

/* --------------------------------------------------------------------------*/

static uint8_t Modem_EventFlagsCb[OS_EVENTFLAGS_CB_SIZE] __ALIGNED(4) EVENTFLAGS_CC_ATTR;

const osEventFlagsAttr_t Modem_EventFlags_Attr = {
  .name    = "Modem Wait",
  .cb_mem  = Modem_EventFlagsCb,
  .cb_size = sizeof(Modem_EventFlagsCb)
};

/* --------------------------------------------------------------------------*/

static uint8_t BufList_MutexCb[OS_MUTEX_CB_SIZE] __ALIGNED(4) MUTEX_CC_ATTR;

const osMutexAttr_t BufList_Mutex_Attr = {
  .name      = "BufList",
  .attr_bits = osMutexPrioInherit,
  .cb_mem    = BufList_MutexCb,
  .cb_size   = sizeof(BufList_MutexCb)
};
