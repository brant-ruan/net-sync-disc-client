#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDE

/* include */
#include "openssl/md5.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h> // no such file on *nix
#include <time.h>
#include <winsock2.h>
//#include <windows.h>

/* define */
#define MYERROR         -1
#define OK              0

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

#define BUF_SIZE        255

/* typedef */
typedef int Status;
typedef unsigned short portType;
/* variable */
struct logInfo{
    char time[TIME_STR_LEN + 2]; // "2016-12-21 08:15:59 " (one whitespace is added)
    char *message;
    int message_len;
};

/* function definition */
Status MD5Str(char *md5, char *str, int str_len);
Status MD5File(char *md5, char *filename);
Status errHandler(char *func_name, char *err_msg, int exit_flag);
int optSel();
int InputUsername(char *username, int *username_len, void (*Func)(void));
int InputPassword(char *password, int *password_len, void (*Func)(void));
Status Login(char* username, char* password, SOCKET *sClient, portType *slisten);
Status Signup(char* username, char* password, SOCKET *sClient, portType *slisten);
Status errMessage(const char *msg);
Status Identify(char *username, char *password_md5, int username_len, SOCKET *sClient, portType *slisten);
Status AddUser(char *username, char *password_md5, int username_len, SOCKET *sClient, portType *slisten);
Status Log(struct logInfo *log_info, char *username);

/* Protocol */
#define PRO_LOGIN       'A'
#define PRO_SIGNUP      'B'
#define PRO_LOGOUT      'D'
#define PRO_BEAT        'E'
#define PRO_JSON        'F'
#define PRO_GET         'G'
#define PRO_POST        'H'
#define PRO_DEL         'I'
#define PRO_RENAME      'J'

#endif // SYNC-DISC-CLIENT_H_INCLUDED
