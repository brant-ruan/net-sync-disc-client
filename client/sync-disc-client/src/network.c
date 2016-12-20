#include "client.h"

unsigned short DATA_PORT = 1500; // data port
unsigned short CTRL_PORT = 1501; // control port
unsigned short BEAT_PORT = 1502; // heart beat port

/*
 * Function:
 *  communicate with server and identify the user
 * Used by login.c
 */
Status Identify(char *username, char *password_md5, int username_len)
{
    return OK;
}

Status AddUser(char *username, char *password_md5, int username_len)
{
    return OK;
}
