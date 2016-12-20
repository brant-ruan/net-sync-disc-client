#include "client.h"

int InputUsername(char *username, int *username_len, void (*Func)(void))
{
    int i;
    while(1){
        system("cls");
        Func();
        printf("\n+-------> Username: ");

        fflush(stdin);

        for(i = 0; i < USERNAME_MAX; i++){
            if((username[i] = fgetc(stdin)) == '\n'){
                username[i] = '\0';
                break;
            }
        }
        username[i] = '\0';
        if(strlen(username) >= USERNAME_MIN){
            *username_len = strlen(username);
            break;
        }
    }

    return OK;
}

int InputPassword(char *password, int *password_len, void (*Func)(void))
{
    int i;
    while(1){
        system("cls");
        Func();
        printf("\n+-------> Password: ");

        fflush(stdin);

        for(i = 0; i < PASSWORD_MAX; i++){
            if((password[i] = fgetc(stdin)) == '\n'){
                password[i] = '\0';
                break;
            }
        }
        password[i] = '\0';
        if(strlen(password) >= PASSWORD_MIN){
            *password_len = strlen(password);
            break;
        }
    }

    return OK;
}
