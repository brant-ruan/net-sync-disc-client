#include "client.h"

#define UP_OK 1 // password includes upper letter
#define LO_OK 2 // password includes lower letter
#define NU_OK 4 // password includes number
#define SP_OK 8 // password includes special sign

const char username_obey[] = \
    "[username]:\n\tlength: 6-12\n\tcharacter: [0-9]/[a-Z]/[A-Z]/_ are allowed";

const char password_obey[] = \
    "[password]:\n\tlength: 8-20\n\tmust include:\n\tupper letter\n\tlower letter\n\tnumber\n\tspecial sign(only ~!@#$%^&*+_|?- are allowed)";

const char special_sign[] = \
    "~!@#$%^&*+_|?-";

/*
 * Function:
 *  judge whether username is valid
 */
Status UsernameIsValid(char *username, int username_len)
{
    if(username_len < USERNAME_MIN)
        return MYERROR;

    int i;
    for(i = 0; i < username_len; i++){
        if(username[i] <= 'z' && username[i] >= 'a')
            continue;
        if(username[i] <= 'Z' && username[i] >= 'A')
            continue;
        if(username[i] <= '9' && username[i] >= '0')
            continue;
        if(username[i] == '_')
            continue;
        break;
    }

    if(i == username_len)
        return OK;
    else
        return MYERROR;
}

Status InSpecialSign(char chr)
{
    int i;
    int spec_len = strlen(special_sign);
    for(i = 0; i < spec_len; i++){
        if(chr == special_sign[i])
            return OK;
    }
    return MYERROR;
}

/*
 * Function:
 *  judge whether password is valid
 */
Status PasswordIsValid(char *password, int password_len)
{
    if(password_len < PASSWORD_MIN)
        return MYERROR;

    char flag = 0x00;

    int i;
    for(i = 0; i < password_len; i++){
        if(password[i] <= 'z' && password[i] >= 'a'){
            flag |= LO_OK;
            continue;
        }
        if(password[i] <= 'Z' && password[i] >= 'A'){
            flag |= UP_OK;
            continue;
        }
        if(password[i] <= '9' && password[i] >= '0'){
            flag |= NU_OK;
            continue;
        }
        if(InSpecialSign(password[i]) == OK){
            flag |= SP_OK;
            continue;
        }
        break;
    }

    if(i < password_len) // password includes other characters
        return MYERROR;
    if((flag & LO_OK) && (flag & SP_OK) && (flag & UP_OK) && (flag & NU_OK))
        return OK;

    return MYERROR;
}

/* Func is a function pointer for prompt */
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
        fflush(stdin);
        username[i] = '\0';
        *username_len = strlen(username);
        if(UsernameIsValid(username, *username_len) == OK)
            break;
        else{
            errMessage(username_obey);
            getchar();
        }
    }

    return OK;
}

/* Func is a function pointer for prompt */
int InputPassword(char *password, int *password_len, void (*Func)(void))
{
    int i;
    while(1){
        system("cls");
        Func();
        printf("\n+-------> Password: ");

        fflush(stdin);

        for(i = 0; i < PASSWORD_MAX; i++){
 // here I use getch() to hide the password when the user input
 // And if you press Ctrl + c here, you can return to the main prompt (optSel)
            if((password[i] = getch()) == '\r'){
                password[i] = '\0';
                break;
            }
            if(password[i] == '\b'){
                if(i > 0){
                    password[i] = 0;
                    i -= 2;
                    printf("\b \b");
                }
                else
                    i--;
            }
            else if(password[i] <= '~' && password[i] >= ' ')
                printf("*");
            else if(password[i] == 0x03) // ctrl + c
                return MYERROR;
            else
                i--;
        }
        fflush(stdin);
        password[i] = '\0';
        *password_len = strlen(password);
        if(PasswordIsValid(password, *password_len) == OK){
            break;
        }
        else{
            errMessage(password_obey);
            getchar();
        }
    }

    return OK;
}
