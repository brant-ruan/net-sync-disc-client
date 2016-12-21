#include "client.h"

// enough to hold strlen("./log/") + strlen(username) + strlen(".log")
#define LOG_PATH_LEN USERNAME_MAX + 6 + 4

/*
 * Function:
 *  write logs into user's log file
 *  mainly used by network.c
 */
Status Log(struct logInfo *log_info, char *username)
{
    char log_path[LOG_PATH_LEN + 1] = "./log/";
    if(username)
        strncat(log_path, username, USERNAME_MAX);
    strcat(log_path, ".log");

    FILE *fp;
    fp = fopen(log_path, "a");
    fwrite(log_info->time, sizeof(char), TIME_STR_LEN + 1, fp); // '1' is for the whitespace

    fwrite(log_info->message, sizeof(char), log_info->message_len, fp);
    fclose(fp);

    return OK;
}
