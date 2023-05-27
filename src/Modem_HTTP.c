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

#include "Modem_HTTP.h"
#include <stdio.h>
#include <string.h>

/* ----------------------------------------------------------------------------- 
 * This function generates an HTTP header for a GET or POST request based on the
 * given parameters and writes it to the output buffer.
 * 
 * Parameters:
 *   out: a pointer to the output buffer where the header will be written
 *   header: a pointer to an HTTP_HeaderTypeDef structure that contains the
 *           information for the header, such as url, method, fields, etc.
 * 
 * Returns:
 *   The number of bytes written to the output buffer.
 * 
 * Example usage:
 *   HTTP_HeaderTypeDef header;
 *   header.url = "http://example.com/index.html";
 *   header.method = HTTP_HEADER_METHOD_GET;
 *   header.nfield = 0;
 *   header.seprator = "\r\n";
 *   char buffer[256];
 *   uint32_t n = HTTPHeader(buffer, &header);
 * -------------------------------------------------------------------------- */
uint32_t HTTPHeader(char *out, HTTP_HeaderTypeDef * header){
  uint32_t n = 0, cntr;
  char *host = strstr(header->url, "//") + 2; // extract the host name from the url
  char *path = strchr(host, '/'); // extract the path from the url
  // char path_buffer = *path;
  if(!header->seprator) 
  	header->seprator = "\n"; // use a default separator if none is given
  n += sprintf (&out[n], "%s %s", (header->method == HTTP_HEADER_METHOD_POST)? "POST": "GET", path); // write the request line
  n += sprintf (&out[n], " HTTP/1.1%s", header->seprator); // write the protocol version and separator
  // path[0] = 0; /// generAte url
  n += sprintf (&out[n], "Host: %.*s%s", path - host, host, header->seprator); // write the host field
  // path[0] = path_buffer; /// generAte url
  for(cntr = 0; cntr < header->nfield; cntr++){ // write any additional fields
    n += sprintf (&out[n], "%s: %s%s", header->fields[cntr].name, header->fields[cntr].value, header->seprator);
  }
  if(header->content_length) 
  	n += sprintf (&out[n], "Content-Length: %d%s", header->content_length, header->seprator); // write the content length field if needed
  n += sprintf (&out[n], "%s", header->seprator); // write an empty line to indicate the end of the header
  return n; // return the number of bytes written
}




























