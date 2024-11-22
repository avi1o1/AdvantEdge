#ifndef ERRORS_H
#define ERRORS_H

#include "defs.h"
#include "colors.h"

#define NUM_ERRORS 29

// Error codes [Index is the code]
extern char *error_messages[NUM_ERRORS];

// Error handling functions
// @brief Logs an error message caused by the naming server to a file
// @param error_code: The error code
// @return void
void log_NM_error(int error_code);

// @brief Logs an error message caused by the client
// @param error_code: The error code
// @return void
void log_CL_error(int error_code);

// @brief Logs an error message caused by the storage server
// @param error_code: The error code
// @return void
void log_SS_error(int error_code);

#endif // ERRORS_H