#include "client.h"

/*
 * Function:
 *  fprintf(stderr, "[%s] %s\n", func_name, err_msg);
 * If exit_flag is enabled, then the program will exit;
 *  else, the function just returns OK.
 * P.S.
 ** This function is just used for developers to debug. For the convenient of user, use errMessage **
 */
Status errHandler(char *func_name, char *err_msg, int exit_flag)
{
    if(func_name && err_msg)
        fprintf(stderr, "[%s] %s\n", func_name, err_msg);
    if(exit_flag == EXIT)
        exit(exit_flag);

    return exit_flag;
}

/*
 * Function:
 *  This is used to indicate user about errors
 */
Status errMessage(const char *msg)
{
    if(msg)
        fprintf(stderr, "\n%s\n", msg);

    return OK;
}
