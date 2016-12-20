#include "client.h"

void SignupPrompt()
{
    printf("\n+---------------------------------------------------------------+");
    printf("\n+                    Net Sync Disc - Sign up                    +");
    printf("\n+---------------------------------------------------------------+");
    printf("\n+                                                               +");
    printf("\n+");
}

Status Signup(char *username, char *password)
{
    int username_len;
    int password_len;

    InputUsername(username, &username_len, SignupPrompt);

    InputPassword(password, &password_len, SignupPrompt);

    char repeat_pass[PASSWORD_MAX + 1];

    char password_md5[MD5_CHAR_LEN + 1];

    MD5Str(password_md5, password, password_len);

    if(AddUser(username, password_md5, username_len) == ERROR)
        return ERROR;

    return OK;
}
