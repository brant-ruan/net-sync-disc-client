/**
 * Program:
 *  Client for net-sync-disc
 * Author:
 *  brant-ruan
 * Beginning date:
 *  2016-12-19
 * P.S.
 *  - MD5 is not safe now, but we just use it to calculate file completeness
 *  - cJSON is used to store and deliver the information of files and directories
 **/

#include "client.h"

char username[USERNAME_MAX + 1] = {0};
char password[PASSWORD_MAX + 1] = {0};

Status main(int argc, char **argv)
{

    int opt_sel;
    /* Prompt and ask for an option */
    opt_sel = optSel();

    switch(opt_sel){
    case OP_LOGIN:
        Login(username, password);
        break;
    case OP_SIGNUP:
        Signup(username, password);
        break;
    case OP_QUIT:
        exit(OK);
        break;
    default:
        break;
    }

    getchar();

	return OK;
}
