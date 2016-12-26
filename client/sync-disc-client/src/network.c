#include "client.h"

// #pragma comment(lib, "ws2_32.lib")

const portType SERVER_MAIN_PORT = 10000;

// const char SEND_OK = 1;

unsigned long nonblock = NONBLOCK;

const char SERVER_IP[IP_LEN + 1] = "192.168.229.222"; // by default

char UID[MD5_CHAR_LEN + 1] = {0};

/*
 * Function:
 *  use `ipconfig /all > ipconfig_tmp.dat`
 *  then MD5File(UID, ipconfig_tmp.dat)
 *
 */
Status UIDInit()
{
    system("ipconfig /all > ipconfig_tmp.dat");
    MD5File(UID, "ipconfig_tmp.dat");
    system("del ipconfig_tmp.dat");

    return OK;
}

/* initialize WSA */
Status WSAInit(WSADATA *wsaData)
{
	WORD sockVersion = MAKEWORD(2, 2);
	if (WSAStartup(sockVersion, wsaData) != 0) {
		errHandler("WSAInit", "WSAStartup error", NO_EXIT);
		return MYERROR;
	}

	return OK;
}

Status sockConfig(SOCKET *sClient, portType port)
{
    // initialize WSA
	WSADATA wsaData;
	if (WSAInit(&wsaData) == MYERROR)
		return MYERROR;

	*sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*sClient == INVALID_SOCKET){
		errHandler("sockConfig", "socket error", NO_EXIT);
		return MYERROR;
	}
	// nonblock
	if (ioctlsocket(*sClient, FIONBIO, (unsigned long *)&nonblock) == SOCKET_ERROR){
		errHandler("sockConfig", "ioctlsocket error", NO_EXIT);
		closesocket(*sClient);
		return MYERROR;
	}

	// connect with server
	struct sockaddr_in sin; // you must add struct or gcc will report error
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.S_un.S_addr = inet_addr(SERVER_IP);
	fd_set wfd;
	fd_set rfd;
	int sel;
	printf("connect?\n");
    if(connect(*sClient, (struct sockaddr *)&sin, sizeof(sin)) == SOCKET_ERROR){
        while(1){
            FD_ZERO(&wfd);
            FD_ZERO(&rfd);
            FD_SET(*sClient, &rfd);
            FD_SET(*sClient, &wfd);
            sel = select(0, &rfd, &wfd, NULL, 0);
            if(sel <= 0 || FD_ISSET(*sClient, &rfd)){
                errHandler("sockConfig", "connect error", NO_EXIT);
                closesocket(*sClient);
                return MYERROR;
            }
            if(FD_ISSET(*sClient, &wfd))
                break;
        }
    }
    printf("connect-hello\n");
	return OK;
}

/*
 * Function:
 *  communicate with server and identify the user
 * Used by login.c and signup.c
 * pro_type is PRO_LOGIN or PRO_SIGNUP
 */
Status Identify(char *username, char *password_md5, int username_len, SOCKET *sClient, portType *slisten, char pro_type)
{
    UIDInit();

    if(sockConfig(sClient, SERVER_MAIN_PORT) == MYERROR){
        errHandler("Identify", "sockConfig error", NO_EXIT);
        WSACleanup();
        return MYERROR;
    }
    /* communicate with server */
    char sendbuf[BUF_SIZE] = {0};
    int sendbuf_len = 0;
    char recvbuf[BUF_SIZE] = {0};
    username[username_len] = 0;
    // A\r\n[username]\r\n[md5(password)]\r\n[UID]\r\n\r\n
    sprintf(sendbuf, "%c%s%s%s%s%s%s%s", pro_type, SEPARATOR, \
            username, SEPARATOR, password_md5, SEPARATOR, UID, TERMINATOR);
    char send_flag = 0;
    sendbuf_len = strlen(sendbuf);
    // write log
    struct logInfo log;
    int sel;
    fd_set rfd;
    fd_set wfd;
    int len;
    while(1){
        FD_ZERO(&rfd);
        FD_ZERO(&wfd);
        FD_SET(*sClient, &rfd);
        FD_SET(*sClient, &wfd);
        sel = select(0, &rfd, &wfd, NULL, 0);
		if (sel == SOCKET_ERROR) {
			errHandler("Identify", "select error", NO_EXIT);
			closesocket(*sClient);
			return MYERROR;
		}
		if(send_flag == 1 && FD_ISSET(*sClient, &rfd)){
            len = recv(*sClient, recvbuf, BUF_SIZE, 0);
            if(len == SOCKET_ERROR){
                errHandler("Identify", "send error", NO_EXIT);
                continue;
            }
            recvbuf[len] = 0;
            log.message = recvbuf;
            log.message_len = len;
            timeGen(log.logtime);
            Log(&log, username); // c -> s [log]
            break;
		}
        if(send_flag == 0 && FD_ISSET(*sClient, &wfd)){
            len = send(*sClient, sendbuf, strlen(sendbuf), 0);
            if(len == SOCKET_ERROR){
                errHandler("Identify", "send error", NO_EXIT);
                continue;
            }
            log.message = sendbuf;
            log.message_len = sendbuf_len;
            timeGen(log.logtime);
            Log(&log, username); // s -> c [log]
            send_flag = 1;
        }
    }
// I have not judged whether the recvbuf length is valid...
// A\r\nY\r\n
    if(recvbuf[0] == pro_type && recvbuf[3] == 'Y'){
        *slisten = atoi(&recvbuf[strlen("A\r\nY\r\n")]);
        return OK;
    }

    return MYERROR;
}

/* remember that after InitSync you need set INITSYNC=1 In conf */
Status InitSync(char *username, SOCKET *CTRLsock, SOCKET *DATAsock, char *config_path)
{
    // if Initial sync has been done , then return directly
    Status done_flag = IsInitSyncDone(config_path);
    if(done_flag == ERROR){
        errHandler("InitSync", "InitSyncDone error", NO_EXIT);
        return ERROR;
    }
    else if(done_flag == YES)
        return OK;

    struct fileInfo file_info;

    // Firstly, client should generate a (filename,md5) list as ./metadata/username-list.data

    // Before the normal transportation, client should check whether there remains a file named as [username-remain.data] in ./metadata/
    // if there is, that means the last file it received last time is incomplete because of network problems,
    // so it will ask the server to provide the rest of that file
    // SERVER WILL ALSO DO THIS JOB

    // if the file size is 0, then ignore it


    return OK;
}

// RTSync - Real Time Sync
/* log out will happen in this function */
Status RTSync(char *username, SOCKET *CTRLsock, SOCKET *DATAsock, char *config_path)
{

    return OK;
}
