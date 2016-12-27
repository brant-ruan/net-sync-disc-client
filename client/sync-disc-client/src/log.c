#include "client.h"

// enough to hold strlen("./log/") + strlen(username) + strlen(".log")
#define LOG_PATH_LEN USERNAME_MAX + 6 + 4

// to generate the time for log
Status timeGen(char *logtime)
{
    time_t ti = time(0);
    struct tm *t = localtime(&ti);
    sprintf(logtime, "%04d-%02d-%02d %02d:%02d:%02d ", \
            t->tm_year + 1900, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    return OK;
}

/*
 * Function:
 *  write logs into user's log file
 *  mainly used by network.c
 */
Status Log(char *message, char *username)
{
    char log_path[LOG_PATH_LEN + 1] = "./log/";
    if(username)
        strncat(log_path, username, USERNAME_MAX);
    strcat(log_path, ".log");

    FILE *fp;
    fp = fopen(log_path, "a+");
    struct logInfo log_info;
    log_info.message = message;
    timeGen(log_info.logtime);
    log_info.message_len = strlen(log_info.message);
    fwrite(log_info.logtime, sizeof(char), TIME_STR_LEN + 1, fp); // '1' is for the whitespace
    fwrite(log_info.message, sizeof(char), log_info.message_len, fp);
    fflush(fp);
    fclose(fp);

    return OK;
}
