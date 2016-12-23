#include "client.h"

void SignupPrompt()
{
    printf("\n+---------------------------------------------------------------+");
    printf("\n+                    Net Sync Disc - Sign up                    +");
    printf("\n+---------------------------------------------------------------+");
    printf("\n+                                                                ");
    printf("\n+");
}

Status Signup(char *username, char *password, SOCKET *sClient, portType *slisten)
{
    int username_len;
    int password_len;

Label_Signup:
    InputUsername(username, &username_len, SignupPrompt);

    if(InputPassword(password, &password_len, SignupPrompt) == MYERROR)
        return MYERROR;

    int repeat_len;
    char repeat_pass[PASSWORD_MAX + 1];

    if(InputPassword(repeat_pass, &repeat_len, SignupPrompt) == MYERROR)
        return MYERROR;

    if(strcmp(password, repeat_pass)){
        errMessage("Passwords you input are not the same");
        getchar();
        goto Label_Signup;
    }

    char password_md5[MD5_CHAR_LEN + 1];

    MD5Str(password_md5, password, password_len);

    if(AddUser(username, password_md5, username_len, sClient, slisten) == MYERROR)
        return MYERROR;

    return OK;
}
