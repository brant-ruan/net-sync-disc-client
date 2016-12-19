#include "client.h"

/*
 * Function:
 *  fprintf(stderr, "[%s] %s\n", func_name, err_msg);
 * If exit_flag is enabled, then the program will exit;
 *  else, the function just returns OK.
 */
Status errHandler(char *func_name, char *err_msg, int exit_flag)
{
    if(func_name && err_msg)
        fprintf(stderr, "[%s] %s\n", func_name, err_msg);
    if(exit_flag == EXIT)
        exit(exit_flag);

    return exit_flag;
}
