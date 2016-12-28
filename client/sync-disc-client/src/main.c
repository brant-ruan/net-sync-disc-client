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

    SOCKET CTRLsock_send;
    // SOCKET BEATsock;
    SOCKET DATAsock_send;
    SOCKET CTRLsock_recv;
    SOCKET DATAsock_recv;
    portType slisten;

    switch(opt_sel){
    case OP_LOGIN:
        if(Login(username, password, &CTRLsock_send, &slisten) == MYERROR){
            errMessage("[username] or [password] not correct.");
            getchar();
            WSACleanup();
            closesocket(CTRLsock_send);
            goto Label_begin;
        }
        break;
    case OP_SIGNUP:
        if(Signup(username, password, &CTRLsock_send, &slisten) == MYERROR){
            errMessage("[username] has been used or you canceled signup.");
            getchar();
            WSACleanup();
            closesocket(CTRLsock_send);
            goto Label_begin;
        }
        break;
    case OP_QUIT:
        exit(OK);
        break;
    default:
        break;
    }
    // configure socket
    if(sockConfig(&DATAsock_send, slisten) == MYERROR){
        errHandler("main", "ConfigDataSock error", NO_EXIT);
        goto Label_end;
    }
    if(sockConfig(&CTRLsock_recv, slisten) == MYERROR){
        errHandler("main", "ConfigDataSock error", NO_EXIT);
        goto Label_end;
    }
    if(sockConfig(&DATAsock_recv, slisten) == MYERROR){
        errHandler("main", "ConfigDataSock error", NO_EXIT);
        goto Label_end;
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

    if(ShowRemoteDir(username, &CTRLsock_send, &DATAsock_send, &CTRLsock_recv, &DATAsock_recv, remote_meta_path) == MYERROR){
        errHandler("main", "ShowRemoteDir error", NO_EXIT);
        goto Label_end;
    }

    if(InitSync(username, &CTRLsock_send, &DATAsock_send, &CTRLsock_recv, &DATAsock_recv, config_path, remote_meta_path) == MYERROR){
        errHandler("main", "InitSync error", NO_EXIT);
        goto Label_end;
    }

    /* real time sync */
    if(RTSync(username, &CTRLsock_send, &DATAsock_send, &CTRLsock_recv, &DATAsock_recv, config_path) == MYERROR){
    /* only when error happens RTSync return MYERROR,
       or it will keep an iteration until user asks to log out */
        errHandler("main", "RTSync", NO_EXIT);
        goto Label_end;
    }


Label_end:
    unlink(remote_meta_path);
    closesocket(CTRLsock_send);
    closesocket(DATAsock_send);
    closesocket(CTRLsock_recv);
    closesocket(DATAsock_recv);
    WSACleanup();

	return OK;
}
