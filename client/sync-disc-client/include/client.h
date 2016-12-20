#ifndef CLIENT_H_INCLUDED
#define CIENT_H_INCLUDED

/* include */
#include "openssl/md5.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* define */
#define ERROR           -1
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

/* typedef */
typedef int Status;

/* function definition */
Status MD5Str(char *md5, char *str, int str_len);
Status MD5File(char *md5, char *filename);
Status errHandler(char *func_name, char *err_msg, int exit_flag);
int optSel();
int InputUsername(char *username, int *username_len, void (*Func)(void));
int InputPassword(char *password, int *password_len, void (*Func)(void));
Status Login(char* username, char* password);
Status Signup(char* username, char* password);
Status errMessage(char *msg);
Status Identify(char *username, char *password_md5, int username_len);

#endif // SYNC-DISC-CLIENT_H_INCLUDED
