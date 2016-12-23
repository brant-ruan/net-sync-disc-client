#include "client.h"

// #pragma comment(lib, "ws2_32.lib")

const portType DATA_PORT = 1500; // data port
const portType CTRL_PORT = 1501; // control port
const portType BEAT_PORT = 1502; // heart beat port
const portType SERVER_MAIN_PORT = 10000;

const char SEND_OK = 1;

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

Status sockConfig(SOCKET *sClient, unsigned short port)
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
            if(sel <= 0){
                errHandler("sockConfig", "connect error", NO_EXIT);
                closesocket(*sClient);
                return MYERROR;
            }
            if(FD_ISSET(*sClient, &rfd))
                printf("rfd ok,but error\n");
            if(FD_ISSET(*sClient, &wfd))
                printf("ok\n");
                break;
        }
    }
    printf("connect-hello\n");
	return OK;
}

/*
 * Function:
 *  communicate with server and identify the user
 * Used by login.c
 */
Status Identify(char *username, char *password_md5, int username_len, SOCKET *sClient, portType *slisten)
{
    UIDInit();
    printf("hello\n");
    if(sockConfig(sClient, SERVER_MAIN_PORT) == MYERROR){
        errHandler("Identify", "sockConfig error", NO_EXIT);
        WSACleanup();
        return MYERROR;
    }
    printf("hello\n");
    /* communicate with server */
    char sendbuf[BUF_SIZE] = {0};
    char recvbuf[BUF_SIZE] = {0};
    username[username_len] = 0;
    // A\r\n[username]\r\n[md5(password)]\r\n[UID]\r\n\r\n
    sprintf(sendbuf, "%c%s%s%s%s%s%s%s", PRO_LOGIN, SEPARATOR, \
            username, SEPARATOR, password_md5, SEPARATOR, UID, TERMINATOR);
    char send_flag = 0;

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
            break;
		}
        if(send_flag == 0 && FD_ISSET(*sClient, &wfd)){
            len = send(*sClient, sendbuf, strlen(sendbuf), 0);
            if(len == SOCKET_ERROR){
                errHandler("Identify", "send error", NO_EXIT);
                continue;
            }
            send_flag = 1;
        }
    }
// I have not judged whether the recvbuf length is valid...
    if(recvbuf[0] == PRO_LOGIN && recvbuf[3] == 'Y'){
        *slisten = atoi(&recvbuf[7]);
        return OK;
    }

    return ERROR;
}

/*
 * Function:
 *  ask server to add an account
 * Used by signup.c
 */
Status AddUser(char *username, char *password_md5, int username_len, SOCKET *sClient, portType *slisten)
{
    UIDInit();

    if(sockConfig(sClient, CTRL_PORT) == MYERROR){
        errHandler("AddUser", "sockConfig error", NO_EXIT);
        WSACleanup();
        return MYERROR;
    }

    return OK;
}
