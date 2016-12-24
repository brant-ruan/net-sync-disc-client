#include "client.h"


Status Login(char *username, char *password, SOCKET *sClient, portType *slisten)
{
    int username_len;
    int password_len;

    InputUsername(username, &username_len, LoginPrompt);

    if(InputPassword(password, &password_len, LoginPrompt) == MYERROR)
        return MYERROR;

    char password_md5[MD5_CHAR_LEN + 1];

    MD5Str(password_md5, password, password_len);

    if(Identify(username, password_md5, username_len, sClient, slisten, PRO_LOGIN) == MYERROR)
        return MYERROR;

    return OK;
}
