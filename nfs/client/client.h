#ifndef CLIENT_H
#define CLIENT_H

#include "../common/include/dataTypes.h"
#include "../common/include/colors.h"
#include "../common/include/defs.h"
#include "../common/include/network.h"
#include "../common/include/errors.h"

/**
 * @brief Display the available commands to the user
 */
void displayCommands(void);

/**
 * @brief Process user input and handle commands
 * @param sock Socket file descriptor for server communication
 * @return 0 if successful, -1 on error
 */
int handleUserCommands(int sock);


/**
 * @brief Clean the input path by removing redundant components and validating it
 * @param inputPath Raw input path
 * @return Cleaned path
 */
char* cleanPath(const char* inputPath);

/**
 * @brief Parse user input into command components
 * @param input Raw input string from user
 * @param command Buffer to store the parsed command
 * @param isFile Flag to indicate if the command is a file operation
 * @param srcPath Buffer to store the source path
 * @param dstPath Buffer to store the destination path
 * @return 0 if parsing successful, -1 on error
 */
int parseUserInput(char* input, char* command, bool* isFile, char* srcPath, char* dstPath);

/**
 * @brief Stream audio data from the server
 * @param ip IP address of the server
 * @param port Port number of the server
 * @param request Client request
 * @return 0 if successful, -1 on error
 */
int streamAudioData(char* ip, int port, char* request);

/**
 * @brief Getting the ctual response data.
 * @param ip IP address of the server
 * @param port Port number of the server
 * @param request Client request
 */
int getActualData(char *ip, int port, char *request);

/**
 * @brief I wonder what it is?
 * @return Something interesting!
 */
void revenge();

#endif // CLIENT_H