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

/* Used during the generation of file-meta data as a queue*/
Status FileQueuePush(char *dir, FILE *fp)
{
    int len = strlen(dir);
    fseek(fp, 0, SEEK_END);
    fwrite(dir, sizeof(char), len, fp);
    fputc('\n', fp);
    fflush(fp);

    return OK;
}

/* Used during the generation of file-meta data as a queue*/
Status FileQueuePop(char *dir, FILE *fp, int *path_offset)
{
    int i = 0;
    int len;
    char temp;
    fseek(fp, *path_offset, SEEK_SET);
    while((temp = fgetc(fp)) != EOF && temp != '\n'){
        dir[i] = temp;
        i++;
    }
    // Note that is not an error, just arriving at the end of file
    if(temp == EOF)
        return MYERROR;

    dir[i] = '\0';
    len = strlen(dir);
    *path_offset += len + 2; // for the \r\n in the file on Windows
    if(len > BUF_SIZE - 3){
        errHandler("FileQueuePop", "File path is too long (> 252); it will be ignored", NO_EXIT);
        return MYERROR;
    }
    dir[len++] = '/';
    dir[len++] = '*';
    dir[len] = '\0';

    return OK;
}

/* generate a meta-data file in ./local-meta/[username].meta */
Status LocalMetaGen(char *username, char *config_path, char *local_meta_path)
{
    int k = 0;
    int len = 0;
    int path_offset = 0;
    char temp;
    FILE *fp;
    char local_dir_path[BUF_SIZE] = {0};
    fp = fopen(config_path, "r");
    if(fp == NULL){
        errHandler("LocalMetaGen", "fopen error", NO_EXIT);
        return MYERROR;
    }
    fseek(fp, strlen("LOCALDIR=N\n\nINITSYNC=N\n\nPATH="), SEEK_SET);
    // read in the local disc directory path
    while((temp = fgetc(fp)) != EOF && temp != '\n')
        local_dir_path[k++] = temp;
    if(local_dir_path[k - 1] != '/') // add a / at the end of path
        local_dir_path[k++] = '/';
    local_dir_path[k] = '\0';
    len = k; // length
    fclose(fp);

    char stack[] = "./config/stack.txt";
    unlink(stack);
    fp = fopen(stack, "w+");
    if(fp == NULL){
        errHandler("LocalMetaGen", "fopen error", NO_EXIT);
        return MYERROR;
    }

    // open meta file
    sprintf(local_meta_path, "./local-meta/%s.meta", username);
    unlink(local_meta_path); // if the file already exists, unlink it
    FILE *meta_fp;
    meta_fp = fopen(local_meta_path, "w");
    if(meta_fp == NULL){
        errHandler("LocalMetaGen", "fopen error", NO_EXIT);
        return MYERROR;
    }

    long HANDLE;
    struct _finddata_t file;
    struct fileInfo file_info;
    char cur_find_path[2 * BUF_SIZE] = {0};
    char temp_find_path[2 * BUF_SIZE] = {0};
    strcpy(cur_find_path, local_dir_path);
    cur_find_path[len] = '*';
    cur_find_path[len + 1] = '\0';
    k = HANDLE = _findfirst(cur_find_path, &file);
    int i, j;

    while(1){
        k = _findnext(HANDLE, &file);
        if(k == -1){ // current directory is ok
            _findclose(HANDLE);
            if(FileQueuePop(cur_find_path, fp, &path_offset) == MYERROR){
                break; // scan end
            }
            k = HANDLE = _findfirst(cur_find_path, &file);
            continue;
        }
        if(file.attrib & _A_SUBDIR){ // directory
            if(strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0)
                continue;
            else{
                if(strlen(file.name) > BUF_SIZE - 3){
                    errHandler("LocalMetaGen", "filename is too long; it will be ignored", NO_EXIT);
                    continue;
                }
                strcpy(temp_find_path, cur_find_path);
                // delete the '*'
                temp_find_path[strlen(temp_find_path) - 1] = '\0';
                strcat(temp_find_path, file.name);
                FileQueuePush(temp_find_path, fp); // into queue
            }
        }
        else{ // file
            if(file.size == 0 || FileIgnore(file.name) == YES) // 0 byte file is ignored
                continue;
            i = 0;
            j = strlen(local_dir_path) - 1; // delete '*'
            while(1){
                temp = cur_find_path[j + i];
                if(temp == '*' || temp == '\0')
                    break;
                file_info.filename[i] = temp;
                i++;
            }
            strcpy(temp_find_path, cur_find_path);
            temp_find_path[strlen(temp_find_path) - 1] = '\0';
            strcat(temp_find_path, file.name);
            strcpy(file_info.filename + i, file.name);
            file_info.filesize = file.size;
            MD5File(file_info.md5, temp_find_path); // use temp_find_path, not file_info.filename!
            len = 0;
            while(len < FILE_INFO_SIZE)
                len += fwrite(&file_info + len, sizeof(char), FILE_INFO_SIZE - len, meta_fp);
            fflush(meta_fp);
        }
    }
    Log("Generate local disc directory meta data - Complete", username);
    fclose(meta_fp);
    fclose(fp);
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
        fprintf(stdout, "+\n+---> %s - %u bytes - md5[%s]\n", meta_file.filename, meta_file.filesize, meta_file.md5);
    }

    fclose(fp);
    return OK;
}

/* This is for expand in the future that user can leave a file-ignore file like .gitnore */
Status FileIgnore(char *filename)
{
    return NO;
}

/* check whether client should transport data for breakpoint */
Status ClientTempRemain(char *username, struct fileInfo *client_file_info, char *tempfile, char *tempfile_info, fileSizeType *tempsize)
{
/*
    username.temp.info is the content of a structure fileInfo
*/
    FILE *fp, *fpp;
    fp = fopen(tempfile, "r");
    fpp = fopen(tempfile_info, "r");
    if(fp == NULL || fpp == NULL){
        fclose(fp);
        fclose(fpp);
        unlink(tempfile);
        unlink(tempfile_info);
        return NO;
    }
    fread(client_file_info, sizeof(char), FILE_INFO_SIZE, fpp);

    fseek(fp, 0, SEEK_END); // calculate file size
    *tempsize = ftell(fp);

    fclose(fp);
    fclose(fpp);
    return YES;
}

/* Generate commands used in initial sync */\
/* flag == YES means that there are temp file */
Status StrategyGen(char *username, Status flag, fileSizeType *tempsize, struct fileInfo *client_file_info, \
                   char *local_meta_path, char *remote_meta_path, char *strategy_path)
{
    FILE *strategy_fp;
    FILE *local_fp;
    FILE *remote_fp;
    strategy_fp = fopen(strategy_path, "w");
    if(strategy_fp == NULL){
        errHandler("StrategyGen", "fopen error", NO_EXIT);
        return MYERROR;
    }
    local_fp = fopen(local_meta_path, "r");
    if(local_fp == NULL){
        errHandler("StrategyGen", "fopen error", NO_EXIT);
        return MYERROR;
    }
    remote_fp = fopen(remote_meta_path, "r");
    if(remote_fp == NULL){
        errHandler("StrategyGen", "fopen error", NO_EXIT);
        return MYERROR;
    }
    struct protocolInfo command;
    int len = 0;
    // breakpoint transportation
    if(flag == YES){
        // G\r\n[md5]\r\n[offset]\r\n\r\n[filename]\r\n[filesize]\r\n\r\n
        sprintf(command.message, "%c%s%s%s%s%s%u%s%u%s", PRO_GET, SEPARATOR, client_file_info->filename, \
                SEPARATOR, client_file_info->md5, SEPARATOR, client_file_info->filesize, \
                SEPARATOR, *tempsize, TERMINATOR);
        command.message_len = strlen(command.message);
        while(len < PROTOCOL_INFO_SIZE){
            len += fwrite(&command + len, sizeof(char), PROTOCOL_INFO_SIZE - len, strategy_fp);
        }
    }

    struct fileInfo local_file;
    struct fileInfo remote_file;
    Status res1;
    Status res2;
    // generate POST
    while(1){
        res1 = fread(&local_file, sizeof(char), FILE_INFO_SIZE, local_fp);
        if(res1 == 0)
            break;
        res2 = SameMD5(username, &local_file, &remote_file, remote_fp);
        if(res == MYERROR){
            errHandler("StrategyGen", "SameMD5 error", NO_EXIT);
            fclose(strategy_fp);
            fclose(local_fp);
            fclose(remote_fp);
            return MYERROR;
        }
        if(res == YES){
            if(){

            }
        }
        else{

        }

    }

    // generate GET
    fseek(local_fp, 0, SEEK_SET);
    fseek(remote_fp, 0, SEEK_SET);
    while(1){
        res = fread(&remote_file, sizeof(char), FILE_INFO_SIZE, remote_fp);
        if(res == 0)
            break;

    }

    fclose(strategy_fp);
    fclose(local_fp);
    fclose(remote_fp);
    return OK;
}
