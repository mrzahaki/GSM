/*  
 *  httpd.h
 *  A header file for HTTP client functions on STM32L4xx microcontrollers.
 *  Author: Hussein Zahaki
 *  Email: mrzahaki@gmail.com
 *  Date: 25 May 2023
 *  
 * -----------------------------------------------------------------------------
 */

// Include guard to prevent multiple inclusion
#ifndef __HTTPD_H
#define __HTTPD_H 

// Include HAL library for STM32L4xx
#include "stm32l4xx_hal.h"
// Uncomment to include RTOS library
// #include "rtx_os.h"

/**
 * \brief A structure to store a name-value pair for an HTTP header field.
 */
typedef struct {
    char *name;   ///< The name of the header field, e.g. "Content-Type"
    char *value;  ///< The value of the header field, e.g. "text/html"
} HTTP_HeaderField;

/**
 * \brief A structure to store the information of an HTTP request or response header.
 */
typedef struct {
    HTTP_HeaderField * fields; ///< An array of header fields, excluding Content-Length
    uint32_t nfield;           ///< The number of header fields in the array
    char * url;                ///< The URL of the request or response, must include http/s scheme
    char * seprator;           ///< The separator character between header fields, default is '\n'
    uint32_t content_length;   ///< The length of the message body in bytes
    unsigned method: 1;        ///< The HTTP method of the request, 0 for GET and 1 for POST

} HTTP_HaderTypeDef;
 
// Define constants for HTTP methods
#define HTTP_HEADER_METHOD_GET     0
#define HTTP_HEADER_METHOD_POST    1

/**
 * \brief A function to generate an HTTP request or response header string from a structure.
 * \param[out] out A pointer to a buffer to store the generated header string
 * \param[in] header A pointer to an HTTP_HaderTypeDef structure with the header information
 * \return The length of the generated header string in bytes, or 0 if an error occurred
 */
extern uint32_t HTTPHeader(char *out, HTTP_HaderTypeDef * header);


#endif //__HTTP_H