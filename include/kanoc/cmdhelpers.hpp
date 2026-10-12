#pragma once

#define CMD_RESET   "\033[0m"
#define CMD_BLACK   "\033[30m"      /* Black */
#define CMD_RED     "\033[31m"      /* Red */
#define CMD_GREEN   "\033[32m"      /* Green */
#define CMD_YELLOW  "\033[33m"      /* Yellow */
#define CMD_BLUE    "\033[34m"      /* Blue */
#define CMD_MAGENTA "\033[35m"      /* Magenta */
#define CMD_CYAN    "\033[36m"      /* Cyan */
#define CMD_WHITE   "\033[37m"      /* White */

#define CMD_BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define CMD_BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define CMD_BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define CMD_BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define CMD_BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define CMD_BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define CMD_BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define CMD_BOLDWHITE   "\033[1m\033[37m"      /* Bold White */


#define CMD_ENDL                            "\n"
#define CMD_TAB                             "\t"
#define CMD_PRINT(CMD_params)               std::cout << CMD_params << CMD_ENDL
#define CMD_COLORED(CMD_params, CMD_col)    CMD_col << CMD_params << CMD_RESET
#define CMD_BRACKETS(CMD_params, CMD_col)   "[" << CMD_COLORED(CMD_params, CMD_col) << "]"
