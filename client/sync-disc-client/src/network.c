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
    char recvbuf[BUF_SIZE] = {0};
    username[username_len] = 0;
    // A\r\n[username]\r\n[md5(password)]\r\n[UID]\r\n\r\n
    sprintf(sendbuf, "%c%s%s%s%s%s%s%s", pro_type, SEPARATOR, \
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
		if(sel > 0){
            if(send_flag == 1 && FD_ISSET(*sClient, &rfd)){
                len = recv(*sClient, recvbuf, BUF_SIZE, 0);
                if(len == SOCKET_ERROR){
                    errHandler("Identify", "send error", NO_EXIT);
                    return MYERROR;
                }
                recvbuf[len] = 0;
                Log(recvbuf, username); // c -> s [log]
                break;
            }
            if(send_flag == 0 && FD_ISSET(*sClient, &wfd)){
                len = send(*sClient, sendbuf, strlen(sendbuf), 0);
                if(len == SOCKET_ERROR){
                    errHandler("Identify", "send error", NO_EXIT);
                    return MYERROR;
                }
                Log(sendbuf, username); // s -> c [log]
                send_flag = 1;
            }
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

/* ask the server to send its net-disc directory and client will show it */
Status ShowRemoteDir(char *username, SOCKET *CTRLsock, SOCKET *DATAsock)
{
    char remote_meta_path[BUF_SIZE] = {0};
    sprintf(remote_meta_path, "./remote-meta/%s.meta", username);
    unlink(remote_meta_path); // if the file already exists, unlink it

    if(TransportRemoteDir(username, CTRLsock, DATAsock, remote_meta_path) == MYERROR){
        errHandler("ShowRemoteDir", "TransportRemoteDir error", NO_EXIT);
        return MYERROR;
    }

    if(DisplayFileInfo(remote_meta_path) == MYERROR){
        errHandler("ShowRemoteDir", "DisplayFileInfo error", NO_EXIT);
        return MYERROR;
    }

    return OK;
}

/* client ask server to send file meta data and store it into a file */
Status TransportRemoteDir(char *username, SOCKET *CTRLsock, SOCKET *DATAsock, char *remote_meta_path)
{
    const char SEND_REQUEST = 1;
    const char RECV_ANSWER  = 2;
    char flag = 0;
    char sendbuf[BUF_SIZE] = {0};
    sprintf(sendbuf, "%c%sR%s", PRO_META, SEPARATOR, TERMINATOR);
    char recvbuf[BUF_SIZE] = {0};
    int recv_meta_size = 0;
    // used within the sync of recv() and fwrite()
    int recv_write_len = 0;
    int recv_already_len = 0;
    int write_already_len = 0;
    int write_ptr = 0;

    FILE *fp;
    if((fp = fopen(remote_meta_path, "w")) == NULL){
        errHandler("TransportRemoteDir", "fopen error", NO_EXIT);
        return MYERROR;
    }

    int sel;
    fd_set rfd;
    fd_set wfd;
    int len;
    while(1){
        FD_ZERO(&rfd);
        FD_ZERO(&wfd);
/*
        FD_SET(fp, &wfd); // is it correct?
        On Windows, fd_set is

typedef struct fd_set {
  u_int  fd_count;
  SOCKET fd_array[FD_SETSIZE];
} fd_set;

    So it seems that FILE *fp is not the same as SOCKET type...
    So you can't use select() for FILE* fp?
*/
        FD_SET(*CTRLsock, &rfd);
        FD_SET(*CTRLsock, &wfd);
        FD_SET(*DATAsock, &rfd);
        sel = select(0, &rfd, &wfd, NULL, 0);
		if (sel == SOCKET_ERROR) {
			errHandler("TransportRemoteDir", "select error", NO_EXIT);
			fclose(fp);
			return MYERROR;
		}
        if(sel > 0){
            if(FD_ISSET(*CTRLsock, &wfd)){
                if(!(flag & SEND_REQUEST)){ // send protocol request
                    len = send(*CTRLsock, sendbuf, strlen(sendbuf), 0);
                    if(len == SOCKET_ERROR){
                        errHandler("TransportRemoteDir", "send error", NO_EXIT);
                        fclose(fp);
                        return MYERROR;
                    }
                    Log(sendbuf, username);
                    flag |= SEND_REQUEST;
                }
            }
            if(FD_ISSET(*CTRLsock, &rfd)){
                if((flag & SEND_REQUEST) && !(flag & RECV_ANSWER)){ // receive protocol answer
                    len = recv(*CTRLsock, recvbuf, BUF_SIZE - 1, 0);
                    if(len == SOCKET_ERROR){
                        errHandler("TransportRemoteDir", "recv error", NO_EXIT);
                        fclose(fp);
                        return MYERROR;
                    }
                    recvbuf[len] = '\0';
                    Log(recvbuf, username);
                    if(recvbuf[0] == PRO_META && recvbuf[3] == 'A'){
                        recv_meta_size = atoi(&recvbuf[strlen("F\r\nA\r\n")]);
                    } // server will send recv_meta_size bytes of meta data
                    else{
                        errHandler("TransportRemoteDir", "Protocol error", NO_EXIT);
                        fclose(fp);
                        return MYERROR;
                    }
                    flag |= RECV_ANSWER;
                }
            }
            if((flag & SEND_REQUEST) && (flag & RECV_ANSWER) && FD_ISSET(*DATAsock, &rfd)){
                if(recv_already_len < recv_meta_size && recv_write_len == 0){
                    recv_write_len = recv(*DATAsock, recvbuf, BUF_SIZE, 0);
                    if(recv_write_len == SOCKET_ERROR){
                        errHandler("TransportRemoteDir", "recv error", NO_EXIT);
                        fclose(fp);
                        return MYERROR;
                    }
                    recv_already_len += recv_write_len;
                }
            }
            if((flag & SEND_REQUEST) && (flag & RECV_ANSWER) && FD_ISSET(fp, &wfd)){
                if(write_already_len < recv_meta_size && recv_write_len > 0){
                    len = fwrite(recvbuf + write_ptr, sizeof(char), recv_write_len, fp);
                    fflush(fp);
                    recv_write_len -= len;
                    write_already_len += len;
                    if(recv_write_len == 0)
                        write_ptr = 0;
                    else
                        write_ptr += len;
                }
                else if(write_already_len >= recv_meta_size) // already write into file ok
                    break;
            }
        }
    }
    fclose(fp);
    return OK;
}

/* remember that after InitSync you need set INITSYNC=1 In conf */
Status InitSync(char *username, SOCKET *CTRLsock, SOCKET *DATAsock, char *config_path)
{
    // if Initial sync has been done , then return directly
    Status done_flag = IsInitSyncDone(config_path);
    if(done_flag == MYERROR){
        errHandler("InitSync", "InitSyncDone error", NO_EXIT);
        return MYERROR;
    }
    else if(done_flag == YES)
        return OK;

    // from here, initial sync will process
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
