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

Status ConfigUser(char *username, char *config_path)
{
    sprintf(config_path, "./config/%s.conf", username);
    FILE *fp;
    fp = fopen(config_path, "r");

    if(fp != NULL){ // already exist
        fclose(fp);
        return OK;
    }

    errMessage("User config file not exist, and will be created");
    fp = fopen(config_path, "w");
    fwrite(default_conf, sizeof(char), strlen(default_conf), fp);
    fflush(fp);
    fclose(fp);

    return OK;
}

Status BindDir(char *config_path)
{
    // \r\n will be handled as \n\n
    FILE *fp;
    fp = fopen(config_path, "r+");
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

/* Check whether the Initial sync has been done */
Status IsInitSyncDone(char *config_path)
{
    FILE *fp;
    fp = fopen(config_path, "r");
    if(fp == NULL){
        errHandler("InitSyncDone", "fopen error", NO_EXIT);
        return ERROR;
    }
    // be aware of the double '\n'
    fseek(fp, strlen("LOCALDIR=N\n\nINITSYNC="), SEEK_SET);
    char temp[2] = {0};
    fread(temp, sizeof(char), 1, fp);
    if(temp[0] == 'Y'){
        fclose(fp);
        return YES;
    }

    fclose(fp);
    return NO;
}

Status SetInitSyncDone(char *username, char *config_path)
{

    return OK;
}

Status UnbindDir(char *config_path)
{
    // LOCALDIR=N

    // INITSYNC=N

    return OK;
}
