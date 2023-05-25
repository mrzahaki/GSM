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
 #include "Modem_Common.h"

 long int MC_StrToInt(char * inp){
	int count = 0;
  int pow = 1;
	long int result = 0;

	while(*inp == ' ') inp++; 
	while(inp[count] != ' ' && inp[count]!=0X00) count++;

		for(;count>0;count--){
			result += ((int)inp[count - 1]-48) * pow;
			pow *= 10;
		}


	return result;
}