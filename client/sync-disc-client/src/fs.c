#include "client.h"

/* deal with filesystem */

/*
 * Function:
 *  If there is [username].conf then just return,
 *  else create [username].conf and configure it by default
 */
char default_conf[] = "LOCALDIR=N\nINITSYNC=N\nPATH=";
/* Note that on Windows printf("xxx\n"); will print \r\n,
   so you do not need to use "\r\n"; just use '\n'
 */

Status ConfigUser(char *username, char *path)
{
    sprintf(path, "./config/%s", username);
    FILE *fp;
    fp = fopen(path, "r");
    if(fp == NULL){
        errMessage("User config file not exist, and will be created");
    }
    fp = fopen(path, "w");
    fwrite(default_conf, sizeof(char), strlen(default_conf), fp);
    fflush(fp);
    fclose(fp);

    return OK;
}

Status BindDir(char *username, char *path)
{
    // \r\n will be handled as \n\n
    FILE *fp;
    fp = fopen(path, "r+");
    char bind_path[BUF_SIZE] = {0};
    char flag[2] = {0};
    int localdir_len = strlen("LOCALDIR=");
    if(fp == NULL){
        errHandler("BindDir", "fopen error", NO_EXIT);
        return MYERROR;
    }
    if(fseek(fp, localdir_len, SEEK_SET) != 0){
        fclose(fp);
        errHandler("BindDir", "fseek error", NO_EXIT);
        return MYERROR;
    }
    fread(flag, sizeof(char), 1, fp);
    int len;
    if(flag[0] != 'Y'){ // No bind-dir
        // ask user to input bind-dir
        printf("+-------> Bind Dir: ");
        fgets(bind_path, BUF_SIZE, stdin);
        len = strlen(bind_path);
        if(bind_path[len - 1] == '\n')
            bind_path[len - 1] = '\0';
        len--;
        // write bind-dir into file
        if(fseek(fp, 0, SEEK_END) != 0){
            fclose(fp);
            errHandler("BindDir", "fseek error", NO_EXIT);
            return MYERROR;
        }
        fwrite(bind_path, sizeof(char), len, fp);
        // change the LOCALDIR=N to LOCALDIR=Y
        if(fseek(fp, localdir_len, SEEK_SET) != 0){
            fclose(fp);
            errHandler("BindDir", "fseek error", NO_EXIT);
            return MYERROR;
        }
        fwrite("Y", sizeof(char), 1, fp);
    }

    fclose(fp);
    return OK;
}

