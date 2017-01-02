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
 *  - MYERROR handle principle:
 *      - all the MYERROR message are printed in the direct function where
 *      - MYERROR happens, and return MYERROR
 *  - Run as administrator :), or the process may have trouble when create or modify files.
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

    SOCKET CTRLsock_client;
    // SOCKET BEATsock;
    SOCKET DATAsock_server;
    SOCKET CTRLsock_server;
    SOCKET DATAsock_client;
    portType slisten;

    switch(opt_sel){
    case OP_LOGIN:
        if(Login(username, password, &CTRLsock_client, &slisten) == MYERROR){
            errMessage("[username] or [password] not correct.");
            getchar();
            WSACleanup();
            closesocket(CTRLsock_client);
            goto Label_begin;
        }
        break;
    case OP_SIGNUP:
        if(Signup(username, password, &CTRLsock_client, &slisten) == MYERROR){
            errMessage("[username] has been used or you canceled signup.");
            getchar();
            WSACleanup();
            closesocket(CTRLsock_client);
            goto Label_begin;
        }
        break;
    case OP_QUIT:
        exit(OK);
        break;
    default:
        break;
    }
    char config_path[BUF_SIZE] = {0};
    char remote_meta_path[BUF_SIZE] = {0};
    if(ConfigUser(username, config_path) == MYERROR){
        errHandler("main", "ConfigUser error", NO_EXIT);
        goto Label_end;
    }
    if(BindDir(username, config_path) == MYERROR){
        errHandler("main", "BindDir error", NO_EXIT);
        goto Label_end;
    }
    /** here you should lock the disc (open a file in disc)
        remains to be done
     **/
    // open a file in the user's disc directory
    FILE *lock_fp;
    if(DiscLockUp(&lock_fp, config_path) == MYERROR){
        errHandler("main", "DiscLockUp error", NO_EXIT);
        goto Label_end;
    }

    // configure socket
    if(sockConfig(&DATAsock_server, slisten) == MYERROR){
        errHandler("main", "ConfigDataSock error", NO_EXIT);
        goto Label_end;
    }
    if(sockConfig(&CTRLsock_server, slisten) == MYERROR){
        errHandler("main", "ConfigDataSock error", NO_EXIT);
        goto Label_end;
    }
    if(sockConfig(&DATAsock_client, slisten) == MYERROR){
        errHandler("main", "ConfigDataSock error", NO_EXIT);
        goto Label_end;
    }

    while(1){
        if(ShowRemoteDir(username, &CTRLsock_client, &DATAsock_server, &CTRLsock_server, &DATAsock_client, remote_meta_path) == MYERROR){
            errHandler("main", "ShowRemoteDir error", NO_EXIT);
            goto Label_end;
        }

        if(InitSync(username, &CTRLsock_client, &DATAsock_server, &CTRLsock_server, &DATAsock_client, config_path, remote_meta_path) == MYERROR){
            errHandler("main", "InitSync error", NO_EXIT);
            goto Label_end;
        }
        unlink(remote_meta_path);
//        sleep(SLEEP_TIME);
    }
    /* real time sync */
    if(RTSync(username, &CTRLsock_client, &DATAsock_server, &CTRLsock_server, &DATAsock_client, config_path) == MYERROR){
    /* only when error happens RTSync return MYERROR,
       or it will keep an iteration until user asks to log out */
        errHandler("main", "RTSync", NO_EXIT);
        goto Label_end;
    }


Label_end:
    DiscLockDown(lock_fp);
    unlink(remote_meta_path);
    closesocket(CTRLsock_client);
    closesocket(DATAsock_server);
    closesocket(CTRLsock_server);
    closesocket(DATAsock_client);
    WSACleanup();

	return OK;
}
