#include "client.h"

void LoginPrompt()
{
    printf("\n+---------------------------------------------------------------+");
    printf("\n+                    Net Sync Disc - Log in                     +");
    printf("\n+---------------------------------------------------------------+");
    printf("\n+                                                                ");
    printf("\n+");
}


Status Login(char *username, char *password)
{
    int username_len;
    int password_len;

    InputUsername(username, &username_len, LoginPrompt);

    if(InputPassword(password, &password_len, LoginPrompt) == ERROR)
        return ERROR;

    char password_md5[MD5_CHAR_LEN + 1];

    MD5Str(password_md5, password, password_len);

    if(Identify(username, password_md5, username_len) == ERROR)
        return ERROR;

    return OK;
}
