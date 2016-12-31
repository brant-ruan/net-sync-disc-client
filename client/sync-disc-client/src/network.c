#include "client.h"

// #pragma comment(lib, "ws2_32.lib")

const portType SERVER_MAIN_PORT = 10000;

// const char SEND_OK = 1;

unsigned long nonblock = NONBLOCK;

const char SERVER_IP[IP_LEN + 1] = "192.168.137.230"; // by default

char UID[MD5_CHAR_LEN + 1] = {0};

const int F_GET = 1;
const int F_POST = 2;
const int F_POST_OK = 4;
const int F_GET_OK = 8;
const int STRATEGY_OK = 16;
const int WAIT_RESPONSE = 32;
const int RESPONSE_Y = 64;
const int RESPONSE_N = 128;
const int F_INIT = 256;
const int F_NEW = 512;
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
Status Identify(char *username, char *password_md5, int username_len, \
                SOCKET *sClient, portType *slisten, char pro_type)
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
Status ShowRemoteDir(char *username, SOCKET *CTRLsock_client, SOCKET *DATAsock_server, \
                     SOCKET *CTRLsock_server, SOCKET *DATAsock_client, char *remote_meta_path)
{
    sprintf(remote_meta_path, "./remote-meta/%s.meta", username);
    unlink(remote_meta_path); // if the file already exists, unlink it

    if(TransportRemoteDir(username, CTRLsock_client, DATAsock_server, CTRLsock_server, DATAsock_client, remote_meta_path) == MYERROR){
        errHandler("ShowRemoteDir", "TransportRemoteDir error", NO_EXIT);
        return MYERROR;
    }

    if(DisplayFileInfo(username, remote_meta_path) == MYERROR){
        errHandler("ShowRemoteDir", "DisplayFileInfo error", NO_EXIT);
        return MYERROR;
    }

    return OK;
}

/* client ask server to send file meta data and store it into a file */
Status TransportRemoteDir(char *username, SOCKET *CTRLsock_client, SOCKET *DATAsock_server, \
                          SOCKET *CTRLsock_server, SOCKET *DATAsock_client, char *remote_meta_path)
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
    if((fp = fopen(remote_meta_path, "wb")) == NULL){
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
    So you can't use select() for FILE* fp (Windows is not linux)
*/
        FD_SET(*CTRLsock_client, &wfd);
        FD_SET(*CTRLsock_client, &rfd);
        FD_SET(*DATAsock_client, &rfd);
        sel = select(0, &rfd, &wfd, NULL, 0);
		if (sel == SOCKET_ERROR) {
			errHandler("TransportRemoteDir", "select error", NO_EXIT);
			fclose(fp);
			return MYERROR;
		}
        if(sel > 0){
            if(FD_ISSET(*CTRLsock_client, &wfd)){
                if(!(flag & SEND_REQUEST)){ // send protocol request
                    len = send(*CTRLsock_client, sendbuf, strlen(sendbuf), 0);
                    if(len == SOCKET_ERROR){
                        errHandler("TransportRemoteDir", "send error", NO_EXIT);
                        fclose(fp);
                        return MYERROR;
                    }
                    Log(sendbuf, username);
                    flag |= SEND_REQUEST;
                }
            }
            if(FD_ISSET(*CTRLsock_client, &rfd)){
                if((flag & SEND_REQUEST) && !(flag & RECV_ANSWER)){ // receive protocol answer
                    len = recv(*CTRLsock_client, recvbuf, BUF_SIZE - 1, 0);
                    if(len == SOCKET_ERROR){
                        errHandler("TransportRemoteDir", "recv error", NO_EXIT);
                        fclose(fp);
                        return MYERROR;
                    }
                    recvbuf[len] = '\0';
                    Log(recvbuf, username);
                    if(recvbuf[0] == PRO_META && recvbuf[3] == 'Y'){
                        recv_meta_size = atoi(&recvbuf[strlen("F\r\nY\r\n")]);
                    } // server will send recv_meta_size bytes of meta data
                    else{
                        errHandler("TransportRemoteDir", "Protocol error", NO_EXIT);
                        fclose(fp);
                        return MYERROR;
                    }
                    flag |= RECV_ANSWER;
                }
            }
            if((flag & SEND_REQUEST) && (flag & RECV_ANSWER) && FD_ISSET(*DATAsock_client, &rfd)){
                if(recv_already_len < recv_meta_size && recv_write_len == 0){
                    recv_write_len = recv(*DATAsock_client, recvbuf, BUF_SIZE, 0);
                    if(recv_write_len == SOCKET_ERROR){
                        errHandler("TransportRemoteDir", "recv error", NO_EXIT);
                        fclose(fp);
                        return MYERROR;
                    }
                    recv_already_len += recv_write_len;
                }
            }
            if((flag & SEND_REQUEST) && (flag & RECV_ANSWER)){
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
    Log("Receive meta-data from server - OK", username);

    return OK;
}

/* remember that after InitSync you need set INITSYNC=1 In conf */
Status InitSync(char *username, SOCKET *CTRLsock_client, SOCKET *DATAsock_server, SOCKET *CTRLsock_server, \
                SOCKET *DATAsock_client, char *config_path, char *remote_meta_path)
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
    // check whether there is ./temp/username.meta
    // if it exists, then ask server to send it
    struct fileInfo client_file_info;
    char tempfile[BUF_SIZE] = {0};
    char tempfile_info[BUF_SIZE] = {0};
    sprintf(tempfile, "./temp/%s.temp", username);
    sprintf(tempfile_info, "./temp/%s.temp.info", username);
    // client_file_info.filesize is currently the size of temp file
    // client_file_info.md5 is the whole file's md5
    // client_file_info.filename makes no sense
    fileSizeType tempsize = 0;
    done_flag = ClientTempRemain(username, &client_file_info, tempfile, tempfile_info, &tempsize);
    if(done_flag == MYERROR){
        errHandler("InitSync", "ClientTempRemain error", NO_EXIT);
        return MYERROR;
    }

    // client should generate a (filename,md5) list as ./local-meta/username.data
    char local_meta_path[BUF_SIZE] = {0};
    if(LocalMetaGen(username, config_path, local_meta_path) == MYERROR){
        errHandler("InitSync", "LocalMetaGen error", NO_EXIT);
        return MYERROR;
    }

    // client compare local meta data with remote meta data and generate strategies
    char strategy_path[BUF_SIZE] = {0};
    sprintf(strategy_path, "./strategy/%s.strategy", username);
    unlink(strategy_path);
    if(StrategyGen(username, done_flag, &tempsize, &client_file_info, local_meta_path, remote_meta_path, strategy_path, config_path) == MYERROR){
        errHandler("InitSync", "StategyGen error", NO_EXIT);
        return MYERROR;
    }

    // client should ponder if it lose connection with server
    if(Sync(username, CTRLsock_client, CTRLsock_server, DATAsock_server, DATAsock_client, strategy_path, config_path) == MYERROR){
        errHandler("InitSync", "Sync error", NO_EXIT);
        return MYERROR;
    }

    if(SetInitSyncDone(username, config_path) == MYERROR){
        errHandler("InitSync", "SetInitSyncDone error", NO_EXIT);
        return MYERROR;
    }

    return OK;
}

/* transport files */
Status Sync(char *username, SOCKET *CTRLsock_client, SOCKET *DATAsock_server, \
            SOCKET *CTRLsock_server, SOCKET *DATAsock_client, char *strategy_path, char *config_path)
{
// ------
    FILE *config_fp;
    config_fp = fopen(config_path, "r");
    if(config_fp == NULL){
        errHandler("Sync", "fopen error", NO_EXIT);
        return MYERROR;
    }
    char disc_base_path[BUF_SIZE] = {0};
    char temp;
    int k = 0;
    fseek(config_fp, strlen("LOCALDIR=N\n\nINITSYNC=N\n\nPATH="), SEEK_SET);
    while((temp = fgetc(config_fp)) != EOF && temp != '\n')
        disc_base_path[k++] = temp;
    disc_base_path[k] = '\0';
    fclose(config_fp);
// ------
    int res = OK;
// ------
    int client_flag = 0;
    int server_flag = 0;

    FlagInit(&client_flag, &server_flag);
// ------
    FILE *client_fp;
    FILE *server_fp;
    FILE *strategy_fp;
    strategy_fp = fopen(strategy_path, "rb");
    if(strategy_fp == NULL){
        errHandler("Sync", "fopen error", NO_EXIT);
        return MYERROR;
    }
// ------
    char client_slice[SLICE_SIZE] = {0}; // temp for file
    char server_slice[SLICE_SIZE] = {0}; // temp for file
    int response_len = strlen("X\r\nX\r\n\r\n");
    char response[BUF_SIZE] = {0};
// ------
    fileSizeType c_filesize;
    fileSizeType client_already;
    fileSizeType s_filesize;
    fileSizeType server_already;
// ------
    int sel;
    fd_set rfd;
    fd_set wfd;
    int len;
    struct protocolInfo command;
    struct protocolInfo server_cmd;
    while(1){
        if((client_flag & STRATEGY_OK))
            break;
        FD_ZERO(&rfd);
        FD_ZERO(&wfd);
        FD_SET(*CTRLsock_client, &rfd);
        FD_SET(*CTRLsock_client, &wfd);
        FD_SET(*CTRLsock_server, &rfd);
        FD_SET(*CTRLsock_server, &wfd);
        FD_SET(*DATAsock_server, &rfd);
        FD_SET(*DATAsock_server, &wfd);
        FD_SET(*DATAsock_client, &rfd);
        FD_SET(*DATAsock_client, &wfd);

        sel = select(0, &rfd, &wfd, NULL, 0);
        if(sel == SOCKET_ERROR){
            errHandler("Sync", "select error", NO_EXIT);
            return MYERROR;
        }
        if(sel == 0)
            continue;
        if(FD_ISSET(*CTRLsock_client, &rfd)){ // reply for client's GET or POST
            if(client_flag & WAIT_RESPONSE){
                client_flag &= ~WAIT_RESPONSE;
                len = recv(*CTRLsock_client, response, response_len, 0);
                if(len == SOCKET_ERROR){
                    errHandler("Sync", "recv error", NO_EXIT);
                    res = MYERROR;
                    goto Label_Sync_end;
                }
                if(((client_flag & F_GET) && (response[0] == PRO_GET)) || \
                   ((client_flag & F_POST) && (response[0] == PRO_POST))){
                    if(response[3] == 'Y'){
                        client_flag |= RESPONSE_Y;
                        client_flag &= ~RESPONSE_N;
                        printf("receive Y\n");
                    }
                    else if(response[3] == 'N'){
                        client_flag |= RESPONSE_N;
                        client_flag &= ~RESPONSE_Y;
                    }
                }
                else{
                    errHandler("Sync", "Protocol error", NO_EXIT);
                    res = MYERROR;
                    goto Label_Sync_end;
                }
            }
        }
        if(FD_ISSET(*CTRLsock_client, &wfd)){ // client sends GET or POST
            if(client_flag & STRATEGY_OK){
                client_flag &= ~F_GET;
                client_flag &= ~F_POST;
                continue; // all is done
            }
            if(((client_flag & F_GET) && (client_flag & F_GET_OK)) || \
               ((client_flag & F_POST) && (client_flag & F_POST_OK)) || \
                (client_flag & RESPONSE_N) || \
               (client_flag & F_INIT)){
                client_flag &= ~F_INIT;
                client_flag &= ~RESPONSE_N;
                printf("-------go______\n");
                len = fread(&command, sizeof(char), PROTOCOL_INFO_SIZE, strategy_fp);
                printf("len: %d\n", len);
                if(len == 0){
                    client_flag |= STRATEGY_OK;
                    continue;
                }
                if(command.message[0] == PRO_GET){
                    client_flag |= F_GET;
                    client_flag &= ~F_GET_OK;
                    client_flag &= ~F_POST;
                }
                else if(command.message[0] == PRO_POST){
                    client_flag |= F_POST;
                    client_flag &= ~F_GET_OK;
                    client_flag &= ~F_GET;
                }
                len = send(*CTRLsock_client, command.message, command.message_len, 0);
                printf("command: %s\n", command.message);
                if(len != command.message_len){
                    errMessage("send protocol - maybe incomplete...");
                }
                client_flag |= WAIT_RESPONSE;
                client_flag |= F_NEW;
            }
        }
        if(FD_ISSET(*CTRLsock_server, &rfd)){ // server send GET or POST
            if(((server_flag & F_GET) && (server_flag & F_GET_OK)) || \
               ((server_flag & F_POST) && (server_flag & F_POST_OK)) || \
               (server_flag & RESPONSE_N) || \
               (server_flag & F_INIT)){
                server_flag &= ~F_INIT;
                server_flag &= ~RESPONSE_N;
                len = recv(*CTRLsock_server, (char *)&server_cmd, PROTOCOL_INFO_SIZE, 0);
                if(len == SOCKET_ERROR){
                    errHandler("Sync", "recv error", NO_EXIT);
                    res = MYERROR;
                    goto Label_Sync_end;
                }
                if(server_cmd.message[0] == PRO_GET){
                    server_flag |= F_GET;
                    server_flag &= ~F_POST;
                    server_flag &= ~F_GET_OK;
                }
                else if(server_cmd.message[0] == PRO_POST){
                    server_flag |= F_POST;
                    server_flag &= ~F_GET;
                    server_flag &= ~F_POST_OK;
                }
                server_flag |= WAIT_RESPONSE;
                server_flag |= F_NEW;
            }
        }
        if(FD_ISSET(*CTRLsock_server, &wfd)){ // reply for server's GET or POST
            if(server_flag & WAIT_RESPONSE){
                server_flag &= ~WAIT_RESPONSE;
                if(server_cmd.message[0] == PRO_GET){
                    Status test_res = HaveSuchFile(username, &server_cmd, disc_base_path);
                    if(test_res == MYERROR){
                        errHandler("Sync", "HaveSuchFile error", NO_EXIT);
                        res = MYERROR;
                        goto Label_Sync_end;
                    }
                    if(test_res == YES){
                        server_flag |= RESPONSE_Y;
                        server_flag &= ~RESPONSE_N;
                    }
                    else{
                        server_flag |= RESPONSE_N;
                        server_flag &= ~RESPONSE_Y;
                    }
                }
                if(server_cmd.message[0] == PRO_POST){
                    errMessage("Oops... Sever [POST] something...");
                    continue;
                }
            }
        }
        if(FD_ISSET(*DATAsock_server, &rfd)){ // data from server (server's POST)
            continue; // in initial sync, server won't send POST
        }
        if(FD_ISSET(*DATAsock_server, &wfd)){ // data to server (server's GET)
            if((server_flag & RESPONSE_Y) && (server_flag & F_GET)){
                if(server_flag & F_NEW){
                    server_flag &= ~F_NEW;
                    server_flag &= ~RESPONSE_Y;
                    if(GETFileOpen2Server(username, &server_fp, &s_filesize, &server_cmd, disc_base_path) == MYERROR){
                        errHandler("Sync", "GETFileOpen2Server error", NO_EXIT);
                        res = MYERROR;
                        goto Label_Sync_end;
                    }
                    server_already = 0;
                }
                len = fread(server_slice, sizeof(char), SLICE_SIZE, server_fp);
                len = send(*DATAsock_server, server_slice, len, 0);
                if(len == SOCKET_ERROR){
                    errHandler("Sync", "send error", NO_EXIT);
                    res = MYERROR;
                    goto Label_Sync_end;
                }
                server_already += len;
                if(server_already >= s_filesize){
                    fclose(server_fp);
                    server_flag |= F_GET_OK;
                }
            }
        }
        if(FD_ISSET(*DATAsock_client, &rfd)){ // data from server (client's GET)
            if((client_flag & RESPONSE_Y) && (client_flag & F_GET)){
                printf("arrive here?\n");
                if(client_flag & F_NEW){
                    client_flag &= ~F_NEW;
                    printf("flag\n");
                    if(GETFileOpen(username, &client_fp, &c_filesize, &command) == MYERROR){
                        errHandler("Sync", "GETFileOpen", NO_EXIT);
                        res = MYERROR;
                        goto Label_Sync_end;
                    }
                    printf("GETFileOpen complete\n");
                    client_already = 0;
                    printf("receive - begin\n");
                }
                len = recv(*DATAsock_client, client_slice, SLICE_SIZE, 0);
                if(len == SOCKET_ERROR){
                    errHandler("Sync", "recv error", NO_EXIT);
                    res = MYERROR;
                    goto Label_Sync_end;
                }
                len = fwrite(client_slice, sizeof(char), len, client_fp);;
                client_already += len;
                if(client_already >= c_filesize){
                    fclose(client_fp);
                    if(MyMoveFile(username, disc_base_path) == MYERROR){
                        errHandler("Sync", "MoveFile error", NO_EXIT);
                        res = MYERROR;
                        goto Label_Sync_end;
                    }
                    client_flag |= F_GET_OK; // finished
                    client_flag &= ~RESPONSE_Y;
                }
            }
        }
        if(FD_ISSET(DATAsock_client, &wfd)){ // data to server (client's POST)
            if((client_flag & RESPONSE_Y) && (client_flag & F_POST)){
                printf("arrive here?\n");
                if(client_flag & F_POST_OK){
                    client_flag &= ~F_POST_OK;
                    if(POSTFileOpen(username, &client_fp, &command, &c_filesize, disc_base_path) == MYERROR){
                        errHandler("Sync", "POSTFileOpen", NO_EXIT);
                        res = MYERROR;
                        goto Label_Sync_end;
                    }
                    client_already = 0;
                }
                len = fread(client_slice, sizeof(char), SLICE_SIZE, client_fp);
                len = send(*DATAsock_client, client_slice, len, 0);
                if(len == SOCKET_ERROR){
                    errHandler("Sync", "send error", NO_EXIT);
                    res = MYERROR;
                    goto Label_Sync_end;
                }
                client_already += len;
                if(client_already >= c_filesize){
                    fclose(client_fp);
                    client_flag |= F_POST_OK; // finished
                }
            }
        }
    }
Label_Sync_end:
//    fclose(client_fp);
//    fclose(server_fp);
    fclose(strategy_fp);
    printf("Sync - OK\n");
    return res;
}

Status FlagInit(int *client_flag, int *server_flag)
{
    *client_flag |= F_GET;
    *client_flag |= F_POST;
//    *client_flag |= F_POST_OK;
//    *client_flag |= F_GET_OK;
    *client_flag |= F_INIT;
    *client_flag &= ~STRATEGY_OK; // important
    *client_flag &= ~RESPONSE_Y;
    *client_flag &= ~RESPONSE_N;
    *client_flag &= ~F_NEW;
    *server_flag |= F_GET;
    *server_flag |= F_POST;
    *server_flag |= F_INIT;
//    *server_flag |= F_GET_OK;
//    *server_flag |= F_POST_OK;
    *server_flag &= ~RESPONSE_Y;
    *server_flag &= ~RESPONSE_N;
    *server_flag &= ~F_NEW;

    return OK;
}

// RTSync - Real Time Sync
/* log out will happen in this function */
Status RTSync(char *username, SOCKET *CTRLsock_client, SOCKET *DATAsock_server, \
              SOCKET *CTRLsock_server, SOCKET *DATAsock_client, char *config_path)
{

    return OK;
}
