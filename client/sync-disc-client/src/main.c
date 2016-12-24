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

    SOCKET CTRLsock;
    // SOCKET BEATsock;
    SOCKET DATAsock;
    portType slisten;

    switch(opt_sel){
    case OP_LOGIN:
        if(Login(username, password, &CTRLsock, &slisten) == MYERROR){
            errMessage("[username] or [password] not correct.");
            getchar();
            WSACleanup();
            closesocket(CTRLsock);
            goto Label_begin;
        }
        break;
    case OP_SIGNUP:
        if(Signup(username, password, &CTRLsock, &slisten) == MYERROR){
            errMessage("[username] has been used or you canceled signup.");
            getchar();
            WSACleanup();
            closesocket(CTRLsock);
            goto Label_begin;
        }
        break;
    case OP_QUIT:
        exit(OK);
        break;
    default:
        break;
    }
    // configure data socket
    if(sockConfig(&DATAsock, slisten) == MYERROR){
        errHandler("main", "ConfigDataSock error", NO_EXIT);
        goto Label_end;
    }

    WelcomePrompt(username);
    char local_path[BUF_SIZE] = {0};
    // if the user delete his conf by himself, then he or she should
    // burden the responsibility by himself
    if(ConfigUser(username) == MYERROR){
        errHandler("main", "ConfigUser error", NO_EXIT);
        goto Label_end;
    }
    // whether the user binds dir
    if(BindDir(username, local_path) == MYERROR){
        errHandler("main", "BindDir error", NO_EXIT);
        goto Label_end;
    }

    if(InitSync(username, &CTRLsock, &DATAsock, local_path) == MYERROR){
        errHandler("main", "InitSync error", NO_EXIT);
        goto Label_end;
    }

    /* real time sync */
    if(RTSync(username, &CTRLsock, &DATAsock, local_path) == MYERROR){
    /* only when error happens RTSync return MYERROR,
       or it will keep an iteration until user asks to log out */
        errHandler("main", "RTSync", NO_EXIT);
        goto Label_end;
    }

Label_end:
    closesocket(CTRLsock);
//    closesocket(BEATsock);
    closesocket(DATAsock);
    WSACleanup();
	return OK;
}
