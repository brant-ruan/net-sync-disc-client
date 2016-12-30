#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDE

/* include */
#include "openssl/md5.h"
//#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h> // no such file on *nix
#include <time.h>
#include <winsock2.h>
#include <io.h> // what is it?
//#include <windows.h>

/* define */
#define MYERROR         -1
#define OK              0
// YES/NO should be different with MYERROR/OK
#define YES             1
#define NO              2

#define USERNAME_MIN    6
#define USERNAME_MAX    12
#define PASSWORD_MIN    8
#define PASSWORD_MAX    20

#define MD5_CHAR_LEN    32
#define MD5_NUM_LEN     16

#define OP_LOGIN        1
#define OP_SIGNUP       2
#define OP_QUIT         3

#define NO_EXIT         0
#define EXIT            1

#define TIME_STR_LEN    19 // 2016-12-21 08:15:59

#define SEPARATOR       "\r\n"
#define TERMINATOR      "\r\n\r\n"

#define IP_LEN          15

#define NONBLOCK        1
#define BLOCK           0

#define BUF_SIZE        256

// when sync, each time 16kb data is sent
#define SEND_SLICE_SIZE 16 * 1024

/* Protocol */
#define PRO_LOGIN       'A'
#define PRO_SIGNUP      'B'
#define PRO_LOGOUT      'D'
#define PRO_UNBIND      'E' // // unbind local directory
#define PRO_META        'F' // file information
#define PRO_GET         'G'
#define PRO_POST        'H'
#define PRO_DEL         'I'
#define PRO_RENAME      'J'
#define PRO_CHGDIR      'K' // change remote directory

/* typedef */
typedef int Status;
typedef unsigned short portType;
typedef unsigned int fileSizeType; //
/* variable */
struct logInfo{
    char logtime[TIME_STR_LEN + 2]; // "2016-12-21 08:15:59 " (one whitespace is added)
    char *message; // point to the message to be logged
    int message_len; // decide the length of message to be logged, so this number should match the real length of message
};

struct fileInfo{
    char filename[BUF_SIZE]; // absolute path relative to the local-bind-dir
    fileSizeType filesize; // file whole size
    char md5[MD5_CHAR_LEN + 1];
    char padding[3];
};

#define FILE_INFO_SIZE  sizeof(struct fileInfo)

struct protocolInfo{
    char message[2 * BUF_SIZE];
    int message_len;
};

#define PROTOCOL_INFO_SIZE  sizeof(struct protocolInfo)

/* function definition */
Status MD5Str(char *md5, char *str, int str_len);
Status MD5File(char *md5, char *filename);
// -----
int optSel();
// -----
int InputUsername(char *username, int *username_len, void (*Func)(void));
int InputPassword(char *password, int *password_len, void (*Func)(void));
// -----
Status Login(char* username, char* password, SOCKET *sClient, portType *slisten);
Status Signup(char* username, char* password, SOCKET *sClient, portType *slisten);
// -----
Status errHandler(char *func_name, char *err_msg, int exit_flag);
Status errMessage(const char *msg);
// -----
Status Identify(char *username, char *password_md5, int username_len, SOCKET *sClient, portType *slisten, char pro_type);
Status sockConfig(SOCKET *sClient, portType port);
Status ShowRemoteDir(char *username, SOCKET *CTRLsock_client, SOCKET *DATAsock_server, SOCKET *CTRLsock_server, SOCKET *DATAsock_client, char *remote_meta_path);
Status TransportRemoteDir(char *username, SOCKET *CTRLsock_client, SOCKET *DATAsock_server, SOCKET *CTRLsock_server, SOCKET *DATAsock_client, char *remote_meta_path);
Status InitSync(char *username, SOCKET *CTRLsock_client, SOCKET *DATAsock_server, SOCKET *CTRLsock_server, SOCKET *DATAsock_client, char *config__path, char *remote_meta_path);
Status RTSync(char *username, SOCKET *CTRLsock_client, SOCKET *DATAsock_server, SOCKET *CTRLsock_server, SOCKET *DATAsock_client, char *config_path);
Status Sync(char *username, SOCKET *CTRLsock_client, SOCKET *DATAsock_server, SOCKET *CTRLsock_server, SOCKET *DATAsock_client, char *strategy_path);
// -----
Status Log(char *message, char *username);
Status timeGen(char *time);
// -----
void SignupPrompt();
void LoginPrompt();
void SelPrompt();
void DiscDirPrompt(char* username);
// -----
Status ConfigUser(char *username, char *config_path);
Status UnbindDir(char *username, char *config_path);
Status BindDir(char *username, char *config_path);
Status DiscLockUp(FILE **lock_fp, char *config_path);
Status DiscLockDown(FILE *lock_fp);
Status IsInitSyncDone(char *config_path);
Status SetInitSyncDone(char *username, char *config_path);
Status FileQueuePush(char *dir, FILE *fp);
Status FileQueuePop(char *dir, FILE *fp, int *path_offset);
Status LocalMetaGen(char *username, char *config_path, char *local_meta_path);
Status DisplayFileInfo(char *username, char *remote_meta_path);
Status FileIgnore(char *filename);
Status ClientTempRemain(char *username, struct fileInfo *client_file_info, char *tempfile, char *tempfile_info, fileSizeType *tempsize);
Status StrategyGen(char *username, Status done_flag, fileSizeType *tempsize, struct fileInfo *client_file_info, \
                   char *local_meta_path, char *remote_meta_path, char *strategy_path, char *config_path);
Status SameName(char *username, struct fileInfo *special_file, struct fileInfo *hold_file, FILE *fp);
Status SameMD5(char *username, struct fileInfo *special_file, struct fileInfo *hold_file, FILE *fp);
Status ChangeName(char *username, struct fileInfo *local_file, char *new_filename, char *config_path);
Status GETStrategy(char *username, FILE *local_fp, FILE *remote_fp, FILE *strategy_fp, Status flag, struct fileInfo *client_file_info, char *config_path);
Status POSTStrategy(char *username, FILE *local_fp, FILE *remote_fp, FILE *strategy_fp);
Status GenGET(char *username, struct protocolInfo *command, struct fileInfo *file_info, fileSizeType offset, FILE *strategy_fp);
Status GenPOST(char *username, struct protocolInfo *command, struct fileInfo *file_info, FILE *strategy_fp);
#endif // SYNC-DISC-CLIENT_H_INCLUDED
