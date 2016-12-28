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
    char message[] = "User config file not exist, and will be created";
    errMessage(message);
    fp = fopen(config_path, "w");
    fwrite(default_conf, sizeof(char), strlen(default_conf), fp);
    fflush(fp);
    fclose(fp);

    // log
    Log(message, username);

    return OK;
}

Status BindDir(char *username, char *config_path)
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

    // log
    char message[2 * BUF_SIZE] = {0};
    sprintf(message, "Bind local directory at: %s", bind_path);
    Log(message, username);

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
        return MYERROR;
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
    FILE *fp;
    fp = fopen(config_path, "r+");
    if(fp == NULL){
        errHandler("SetInitSyncDone", "fopen error", NO_EXIT);
        return MYERROR;
    }
    // be aware of the double '\n'
    fseek(fp, strlen("LOCALDIR=N\n\nINITSYNC="), SEEK_SET);
    fwrite("Y", sizeof(char), 1, fp);


    // log
    Log("Initial sync is done", username);

    fclose(fp);
    return OK;
}

Status UnbindDir(char *username, char *config_path)
{
    if(unlink(config_path) == -1){
        errHandler("UnbindDir", "remove error", NO_EXIT);
        return MYERROR;
    }

    FILE *fp;
    fp = fopen(config_path, "w");
    if(fp == NULL){
        errHandler("UnbindDir", "fopen error", NO_EXIT);
        return MYERROR;
    }

    fwrite(default_conf, sizeof(char), strlen(default_conf), fp);

    // log
    Log("Unbind local directory", username);

    fclose(fp);
    return OK;
}

/* generate a meta-data file in ./local-meta/[username].meta */
Status LocalMetaGen(char *username, char *config_path)
{

    return OK;
}

Status DisplayFileInfo(char *username, char *remote_meta_path)
{
    FILE *fp;
    struct fileInfo meta_file;

    fp = fopen(remote_meta_path, "r");
    if(fp == NULL){
        errHandler("DisplayFileInfo", "fopen error", NO_EXIT);
        return MYERROR;
    }

    DiscDirPrompt(username); // Give a prompt

    while(fread(&meta_file, sizeof(char), FILE_INFO_SIZE, fp) != 0){
        fprintf(stdout, "+\n+---> %s - %d bytes - md5[%s]\n", meta_file.filename, meta_file.filesize, meta_file.md5);
    }

    fclose(fp);
    return OK;
}

/*
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <io.h>
#define MYERROR         -1
#define OK              0
#define YES             1
#define NO              0

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

#define BUF_SIZE        256
typedef int Status;
char default_conf[] = "LOCALDIR=N\nINITSYNC=N\nPATH=";
struct fileInfo{
    char filename[BUF_SIZE]; // absolute path relative to the local-bind-dir
    int filesize; // file whole size
    char md5[MD5_CHAR_LEN + 1];
    char padding[2];
};

#define FILE_INFO_SIZE  sizeof(struct fileInfo)
Status FileQueuePush(char *dir, FILE *fp, int *path_offset)
{
    int len = strlen(dir);
    fseek(fp, 0, SEEK_END);
    fwrite(dir, sizeof(char), len, fp);
    fputc('\n', fp);
    fflush(fp);

    return OK;
}

Status FileQueuePop(char *dir, FILE *fp, int *path_offset)
{
    int i = 0;
    char temp;
    fseek(fp, *path_offset, SEEK_SET);
    printf("ftell: %d\n", ftell(fp));
    printf("this time: ");
    while((temp = fgetc(fp)) != EOF && temp != '\n'){
        printf("%c", temp);
        dir[i] = temp;
        i++;
    }
    printf("\n");
    if(temp == EOF)
        return MYERROR; // Note that is not an error, just arriving at the end of file
    //temp = fgetc(fp); // to read out the last \n

    dir[i] = '\0';
    int len = strlen(dir);
    *path_offset += len + 2;
    if(len > BUF_SIZE - 3){
        printf("error\n");
        return MYERROR;
    }
    else{
        dir[len++] = '/';
        dir[len++] = '*';
        dir[len] = '\0';
    }
    printf("next dir: %s\n", dir);

    return OK;
}

// Status GenLocal(char *)
int main(void)
{
    struct _finddata_t file;
    int k;
    int len;
    int path_offset = 0;
    FILE *fp;
    char dir_path[2 * BUF_SIZE] = {0};
    char stack[] = "./config/stack.txt";
    unlink(stack);
    fp = fopen("./config/stack.txt", "w+");
    long HANDLE;
    char cur_path[BUF_SIZE] = "D:/2linux/network/last/temp/*";
    k = HANDLE = _findfirst(cur_path, &file);
    while(1){
        k = _findnext(HANDLE, &file);
        if(k == -1){
            _findclose(HANDLE);
            if(FileQueuePop(cur_path, fp, &path_offset) == MYERROR){
                printf("end\n");
                break;
            }
            printf("new pop out: %s\n", cur_path);
            k = HANDLE = _findfirst(cur_path, &file);
            continue;
        }
        if(file.attrib & _A_SUBDIR){
            printf("dir: %s\n", file.name);
            if(strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0)
                continue;
            else{
                strcpy(dir_path, cur_path);
                dir_path[strlen(dir_path) - 1] = '\0';
                if(strlen(file.name) > BUF_SIZE - 3){
                    printf("[%s] filename is too long...\n", file.name);
                }
                else{
                    strcat(dir_path, file.name);
                    printf("write in: %s\n", dir_path);
                    FileQueuePush(dir_path, fp, &path_offset);
                }
            }
        }
        else{
            printf("name: %s size: %u [file]\n", file.name, file.size);
        }
    }

    _findclose(HANDLE);
    fclose(fp);
    return 0;
}

*/
