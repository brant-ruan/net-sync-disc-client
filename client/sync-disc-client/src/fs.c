#include "client.h"

/* deal with filesystem */

/*
 * Function:
 *  If there is [username].conf then just return,
 *  else create [username].conf and configure it by default
 */
char default_conf[] = "LOCALDIR=N\nINITSYNC=N\n";

Status ConfigUser(char *username)
{

    return OK;
}

Status BindDir(char *username, char *local_path)
{
    return OK;
}



