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
/* History:
 *  Version 2.0
 *    Changed prefix ARM_DRV -> MOD_DRIVER
 *    Added General return codes definitions
 *  Version 1.10
 *    Namespace prefix ARM_ added
 *  Version 1.00
 *    Initial release
 */

#ifndef MOD_COMMON_H_
#define MOD_COMMON_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "Driver_Common.h"

#define MOD_DRIVER_VERSION_MAJOR_MINOR(major,minor) (((major) << 8) | (minor))

/**
\brief Driver Version
*/
typedef struct _MOD_DRIVER_VERSION {
  uint16_t api;                         ///< API version
  uint16_t drv;                         ///< Driver version
} MOD_DRIVER_VERSION;

/* General return codes */
#define MOD_DRIVER_OK                 0 ///< Operation succeeded 
#define MOD_DRIVER_ERROR             -1 ///< Unspecified error
#define MOD_DRIVER_ERROR_BUSY        -2 ///< Driver is busy
#define MOD_DRIVER_ERROR_TIMEOUT     -3 ///< Timeout occurred
#define MOD_DRIVER_ERROR_UNSUPPORTED -4 ///< Operation not supported
#define MOD_DRIVER_ERROR_PARAMETER   -5 ///< Parameter error
#define MOD_DRIVER_ERROR_SPECIFIC    -6 ///< Start of driver specific errors 

/**
\brief General power states
*/ 
typedef enum _MOD_POWER_STATE {
  MOD_POWER_OFF,                        ///< Power off: no operation possible
  MOD_POWER_LOW,                        ///< Low Power mode: retain state, detect and signal wake-up events
  MOD_POWER_FULL                        ///< Power on: full operation at maximum performance
} MOD_POWER_STATE;


 long int MC_StrToInt(char * inp);


#endif /* DRIVER_COMMON_H_ */