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

/* put username and password here, not in client.h,
 * just to make it more clear
 */
char username[USERNAME_MAX + 1];
char password[PASSWORD_MAX + 1];

Status main(int argc, char **argv)
{
    int opt_sel;
Label_begin:
    memset(username, 0, USERNAME_MAX + 1);
    memset(password, 0, PASSWORD_MAX + 1);
    /* Prompt and ask for an option */
    opt_sel = optSel();

    switch(opt_sel){
    case OP_LOGIN:
        if(Login(username, password) == ERROR){
            errMessage("[username] or [password] not correct.");
            getchar();
            goto Label_begin;
        }
        break;
    case OP_SIGNUP:
        if(Signup(username, password) == ERROR){
            errMessage("[username] has been used or you canceled signup.");
            getchar();
            goto Label_begin;
        }
        break;
    case OP_QUIT:
        exit(OK);
        break;
    default:
        break;
    }

//    Welcome(username);

    getchar();

	return OK;
}
