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
    char opt;
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
Label_bind_dir:
    if(flag[0] != 'Y'){ // No bind-dir
        // ask user to input bind-dir
        printf("+-------> Bind Directory: ");
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
    else{
        printf("+-------> Do you want to bind a new directory? (y/n): ");
        opt = fgetc(stdin);
        if(opt == 'Y' || opt == 'y'){
            flag[0] = 'N';
            fflush(stdin);
            goto Label_bind_dir;
        }
    }

    // log
    Log("Bind or rebind - Complete", username);

    fclose(fp);
    return OK;
}

Status DiscLockUp(FILE **lock_fp, char *config_path)
{
    FILE *config_fp;
    config_fp = fopen(config_path, "r");
    char temp;
    int k = 0;
    fseek(config_fp, strlen("LOCALDIR=N\n\nINITSYNC=N\n\nPATH="), SEEK_SET);
    char disc_path[2 * BUF_SIZE] = {0};
    while((temp = fgetc(config_fp)) != EOF && temp != '\n')
        disc_path[k++] = temp;
    disc_path[k] = '\0';
    strcat(disc_path, "/DISC.LOCK");
    unlink(disc_path);
    *lock_fp = fopen(disc_path, "w");
    if(*lock_fp == NULL){
        errHandler("DiscLockUp", "fopen error", NO_EXIT);
        return MYERROR;
    }

    return OK;
}

Status DiscLockDown(FILE *lock_fp)
{
    fclose(lock_fp);
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
    meta_fp = fopen(local_meta_path, "wb");
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

    fp = fopen(remote_meta_path, "rb");
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

/* Generate commands used in initial sync */
/* flag == YES means that there are temp file */
Status StrategyGen(char *username, Status flag, fileSizeType *tempsize, struct fileInfo *client_file_info, \
                   char *local_meta_path, char *remote_meta_path, char *strategy_path, char *config_path)
{
    FILE *strategy_fp;
    FILE *local_fp;
    FILE *remote_fp;
    strategy_fp = fopen(strategy_path, "wb");
    if(strategy_fp == NULL){
        errHandler("StrategyGen", "fopen error", NO_EXIT);
        return MYERROR;
    }
    local_fp = fopen(local_meta_path, "rb");
    if(local_fp == NULL){
        errHandler("StrategyGen", "fopen error", NO_EXIT);
        fclose(strategy_fp);
        return MYERROR;
    }
    remote_fp = fopen(remote_meta_path, "rb");
    if(remote_fp == NULL){
        errHandler("StrategyGen", "fopen error", NO_EXIT);
        fclose(strategy_fp);
        fclose(local_fp);
        return MYERROR;
    }
    Status ret = OK;
    struct protocolInfo command;
    // breakpoint transportation
    if(flag == YES){
        // G\r\n[md5]\r\n[offset]\r\n\r\n[filename]\r\n[filesize]\r\n\r\n
        if(GenGET(username, &command, client_file_info, *tempsize, strategy_fp) == MYERROR){
            errHandler("StrategyGen", "GenGET error", NO_EXIT);
            ret = MYERROR;
            goto label_Strategy_end;
        }
    }

    if(GETStrategy(username, local_fp, remote_fp, strategy_fp, flag, client_file_info, config_path) == MYERROR){
        errHandler("StrategyGen", "GETStrategy error", NO_EXIT);
        ret = MYERROR;
        goto label_Strategy_end;
    }

    if(POSTStrategy(username, local_fp, remote_fp, strategy_fp) == MYERROR){
        errHandler("StrategyGen", "POSTStrategy error", NO_EXIT);
        ret = MYERROR;
        goto label_Strategy_end;
    }

label_Strategy_end:
    fclose(strategy_fp);
    fclose(local_fp);
    fclose(remote_fp);
    return ret;
}

/* Generate POST strategy */
Status POSTStrategy(char *username, FILE *local_fp, FILE *remote_fp, FILE *strategy_fp)
{
    struct protocolInfo command;
    struct fileInfo local_file;
    struct fileInfo remote_file;
    Status res1;
    Status res2;
    Status res3;
    fseek(local_fp, 0, SEEK_SET);
    fseek(remote_fp, 0, SEEK_SET);

    while(1){
        res1 = fread(&local_file, sizeof(char), FILE_INFO_SIZE, local_fp);
        if(res1 == 0)
            break; // all is generated
        res2 = SameName(username, &local_file, &remote_file, remote_fp);
        if(res2 == MYERROR){
            errHandler("POSTStrategy", "SameName error", NO_EXIT);
            return MYERROR;
        }
        if(res2 == YES){ // has file of same name
            if(strcmp(local_file.md5, remote_file.md5) == 0)
                continue; // same name same MD5
            else{ // same name different MD5
                // local file has been changed name when GET was generated
                sprintf(local_file.filename, "%s.%s", local_file.filename, local_file.md5);
                if(GenPOST(username, &command, &local_file, strategy_fp) == MYERROR){
                    errHandler("POSTStrategy", "GenPOST error", NO_EXIT);
                    return MYERROR;
                }
            }
        }
        else{
            res3 = SameMD5(username, &local_file, &remote_file, remote_fp);
            if(res3 == MYERROR){
                errHandler("POSTStrategy", "SameMD5 error", NO_EXIT);
                return MYERROR;
            }
            if(res3 == YES){ // different file same MD5
                continue; // local file has been changed name when GET was generated
            }
            else{
                if(GenPOST(username, &command, &local_file, strategy_fp) == MYERROR){
                    errHandler("POSTStrategy", "GenPOST error", NO_EXIT);
                    return MYERROR;
                }
            }
        }
    }

    Log("Generate POST strategies - Complete", username);

    return OK;
}

/* Generate GET strategy */
Status GETStrategy(char *username, FILE *local_fp, FILE *remote_fp, \
                   FILE *strategy_fp, Status flag, struct fileInfo *client_file_info, char *config_path)
{
    struct protocolInfo command;
    struct fileInfo local_file;
    struct fileInfo remote_file;
    Status res1;
    Status res2;
    Status res3;
    char new_filename[BUF_SIZE] = {0};

    fseek(local_fp, 0, SEEK_SET);
    fseek(remote_fp, 0, SEEK_SET);
    while(1){
        res1 = fread(&remote_file, sizeof(char), FILE_INFO_SIZE, remote_fp);
        if(res1 == 0) // all is generated
            break;
        res2 = SameName(username, &remote_file, &local_file, local_fp);
        if(res2 == MYERROR){
            errHandler("GETStrategy", "SameName error", NO_EXIT);
            return MYERROR;
        }
        if(res2 == YES){ // has file of same name
            if(strcmp(remote_file.md5, local_file.md5) == 0)
                continue; // same name same MD5
            else{ // same name different MD5
                // change local file's name
                sprintf(new_filename, "%s.%s", local_file.filename, local_file.md5);
                if(ChangeName(username, &local_file, new_filename, config_path) == MYERROR){
                    errHandler("GETStrategy", "ChangeName error", NO_EXIT);
                    return MYERROR;
                }
                // generate GET
                if(GenGET(username, &command, &remote_file, 0, strategy_fp) == MYERROR){
                    errHandler("GETStrategy", "GenGET error", NO_EXIT);
                    return MYERROR;
                }
            }
        }
        else{ // no file of same name
            res3 = SameMD5(username, &remote_file, &local_file, local_fp);
            if(res3 == MYERROR){
                errHandler("GETStrategy", "SameMD5 error", NO_EXIT);
                return MYERROR;
            }
            if(res3 == YES){ // different file same MD5
                if(ChangeName(username, &local_file, remote_file.filename, config_path) == MYERROR){
                    errHandler("GETStrategy", "ChangeName error", NO_EXIT);
                    return MYERROR;
                }
            }
            else{ // no such file and no such MD5
                // if it is the very breakpoint file, skip
                if(flag == YES && strcmp(client_file_info->md5, remote_file.md5) == 0 && \
                   strcmp(client_file_info->filename, remote_file.filename) == 0)
                    continue;
                else{ // generate GET
                    if(GenGET(username, &command, &remote_file, 0, strategy_fp) == MYERROR){
                        errHandler("GETStrategy", "GenGET error", NO_EXIT);
                        return MYERROR;
                    }
                }
            }
        }
    }

    Log("Generate GET strategies - Complete", username);

    return OK;
}

/** Both SameName and SameMD5 specify the file in the same sub-directory **/

/* to find whether there is a file with the same name */
Status SameName(char *username, struct fileInfo *special_file, struct fileInfo *hold_file, FILE *fp)
{
    fseek(fp, 0, SEEK_SET);

    while(fread(hold_file, sizeof(char), FILE_INFO_SIZE, fp) == FILE_INFO_SIZE){
        if(strcmp(special_file->filename, hold_file->filename) == 0)
            return YES;
    }

    return NO;
}
/* to find whether there is a file with the same MD5 */
Status SameMD5(char *username, struct fileInfo *special_file, struct fileInfo *hold_file, FILE *fp)
{
    fseek(fp, 0, SEEK_SET);
    char temp1[BUF_SIZE] = {0};
    char temp2[BUF_SIZE] = {0};
    fileSizeType len;
    int i;
    while(fread(hold_file, sizeof(char), FILE_INFO_SIZE, fp) == FILE_INFO_SIZE){
        if(strcmp(special_file->md5, hold_file->md5) == 0){
            len = strlen(special_file->filename);
            for(i = len - 1; i >=0; i--){
                if(special_file->filename[i] == '/')
                    break;
            }
            strncpy(temp1, special_file->filename, i + 1);
            temp1[i + 1] = '\0';
            len = strlen(hold_file->filename);
            for(i = len - 1; i >=0; i--){
                if(hold_file->filename[i] == '/')
                    break;
            }
            strncpy(temp2, hold_file->filename, i + 1);
            temp2[i + 1] = '\0';
            if(strcmp(temp1, temp2) == 0)
                return YES;
        }
    }

    return NO;
}

Status ChangeName(char *username, struct fileInfo *local_file, char *new_filename, char *config_path)
{
    FILE *fp;
    fp = fopen(config_path, "r");
    if(fp == NULL){
        errHandler("ChangeName", "fopen error", NO_EXIT);
        return ERROR;
    }

    char disc_base_path[2 * BUF_SIZE] = {0};
    char old_name[2 * BUF_SIZE] = {0};
    char new_name[2 * BUF_SIZE] = {0};
    char temp;
    int k = 0;
    fseek(fp, strlen("LOCALDIR=N\n\nINITSYNC=N\n\nPATH="), SEEK_SET);

    while((temp = fgetc(fp)) != EOF && temp != '\n')
        disc_base_path[k++] = temp;
    disc_base_path[k] = '\0';

    fclose(fp);

    strcpy(old_name, disc_base_path);
    strcat(old_name, local_file->filename);
    strcpy(new_name, disc_base_path);
    strcat(new_name, new_filename);
    if(rename(old_name, new_name) == -1){
        errHandler("ChangeName", "rename error", NO_EXIT);
        return MYERROR;
    }

    return OK;
}

Status GenGET(char *username, struct protocolInfo *command, \
              struct fileInfo *file_info, fileSizeType offset, FILE *strategy_fp)
{
    int len = 0;
    sprintf(command->message, "%c%s%s%s%s%s%u%s%u%s", PRO_GET, SEPARATOR, file_info->filename, \
            SEPARATOR, file_info->md5, SEPARATOR, file_info->filesize, \
            SEPARATOR, offset, TERMINATOR);
    command->message_len = strlen(command->message);
    while(len < PROTOCOL_INFO_SIZE){
        len += fwrite(command + len, sizeof(char), PROTOCOL_INFO_SIZE - len, strategy_fp);
    }

    return OK;
}

Status GenPOST(char *username, struct protocolInfo *command, \
              struct fileInfo *file_info, FILE *strategy_fp)
{
    int len = 0;
    sprintf(command->message, "%c%s%s%s%s%s%u%s", PRO_POST, SEPARATOR, file_info->filename, \
            SEPARATOR, file_info->md5, SEPARATOR, file_info->filesize, TERMINATOR);
    command->message_len = strlen(command->message);
    while(len < PROTOCOL_INFO_SIZE){
        len += fwrite(command + len, sizeof(char), PROTOCOL_INFO_SIZE - len, strategy_fp);
    }

    return OK;
}

/* open a file for POST */
Status POSTFileOpen(char *username, FILE **client_fp, struct protocolInfo *command, \
                    fileSizeType *filesize, char *disc_base_path, char *filename)
{
    char file_path[BUF_SIZE] = {0};
    int len;
    strcpy(file_path, disc_base_path);
    len = strlen(file_path);
    int i;
    for(i = 0; command->message[3 + i] != '\r'; i++)
        ;
    strncat(file_path, command->message + 3, i);
    strncpy(filename, command->message + 3, i);
    filename[i] = '\0';
    file_path[len + i] = '\0';
    *client_fp = fopen(file_path, "rb");
    if(*client_fp == NULL){
        errHandler("POSTFileOpen", "fopen error", NO_EXIT);
        return MYERROR;
    }
    fseek(*client_fp, 0, SEEK_END);
    *filesize = ftell(*client_fp);
    fseek(*client_fp, 0, SEEK_SET);
    return OK;
}

/* move complete temp file to user's disc and delete temp.info file */
Status MyMoveFile(char *username, char *disc_base_path)
{
    char temp_path[BUF_SIZE] = {0};
    char temp_info_path[BUF_SIZE] = {0};
    sprintf(temp_path, ".\\temp\\%s.temp", username);
    sprintf(temp_info_path, "./temp/%s.temp.info", username);

    struct fileInfo file;

    FILE *fp;
    fp = fopen(temp_info_path, "rb");
    if(fp == NULL){
        errHandler("MyMoveFile", "fopen error", NO_EXIT);
        return MYERROR;
    }

    fread(&file, sizeof(char), FILE_INFO_SIZE, fp);

    fclose(fp);

    char real_path[BUF_SIZE] = {0};
    strcpy(real_path, disc_base_path);
    strcat(real_path, file.filename);
    char cmd[2 * BUF_SIZE] = {0};
    char mkdir_path[BUF_SIZE] = {0};
    strcpy(mkdir_path, real_path);
    int i = strlen(mkdir_path) - 1;
    int flag = 1;
    for(; i >= 0; i--){
        if(mkdir_path[i] == '/'){
            flag = 0;
            mkdir_path[i] = '\\';
        }
        if(flag == 1)
            mkdir_path[i] = '\0';
    }
    // mkdir
    sprintf(cmd, "mkdir %s > nul", mkdir_path);
    system(cmd);
    // move file
    sprintf(cmd, "move %s %s > nul", temp_path, real_path);
    system(cmd);
    Log(cmd, username); // Log

    unlink(temp_info_path);
    return OK;
}

/* open a temp file for GET */
Status GETFileOpen(char *username, FILE **client_fp, fileSizeType *c_filesize, struct protocolInfo *command)
{
    char temp_path[BUF_SIZE] = {0};
    char temp_info_path[BUF_SIZE] = {0};
    struct fileInfo temp_info;
    fileSizeType offset;

    sprintf(temp_path, "./temp/%s.temp", username);
    sprintf(temp_info_path, "./temp/%s.temp.info", username);
    // open temp file
    *client_fp = fopen(temp_path, "ab+"); // 'a' is for breakpoint transportation
    if(*client_fp == NULL){
        errHandler("GETFileOpen", "fopen error", NO_EXIT);
        return MYERROR;
    }
    GET_Cmd2fileInfo(username, &temp_info, &offset, command);

    *c_filesize = temp_info.filesize - offset;
    FILE *info_fp;
    info_fp = fopen(temp_info_path, "wb");
    if(info_fp == NULL){
        errHandler("GETFileOpen", "fopen error", NO_EXIT);
        return MYERROR;
    }

    fwrite(&temp_info, sizeof(char), FILE_INFO_SIZE, info_fp);
    fclose(info_fp);

    return OK;
}

/* parse information in a GET protocol and stores it in a fileInfo */
Status GET_Cmd2fileInfo(char *username, struct fileInfo *temp_info, \
                        fileSizeType *offset, struct protocolInfo *command)
{
    int i, k;
    for(i = 0; command->message[3 + i] != '\r'; i++){
        temp_info->filename[i] = command->message[3 + i];
    }
    temp_info->filename[i] = '\0';
    k = 0;
    for(; command->message[3 + 2 + i] != '\r'; i++){
        temp_info->md5[k] = command->message[3 + 2 + i];
        k++;
    }
    temp_info->md5[k] = '\0';

    temp_info->filesize = atoi(&(command->message[3 + 2 + 2 + i]));

    for(; command->message[3 + 2 + 2 + i] != '\r'; i++)
        ;
    *offset = atoi(&(command->message[3 + 2 + 2 + 2 + i]));

    return OK;
}

/* Judge whether there is a such file asked by server in client's disc directory */
Status HaveSuchFile(char *username, struct protocolInfo *server_cmd, char *disc_base_path)
{
    struct fileInfo myfile;
    fileSizeType offset;

    GET_Cmd2fileInfo(username, &myfile, &offset, server_cmd);

    char file_path[BUF_SIZE] = {0};
    strcpy(file_path, disc_base_path);
    strcpy(file_path, myfile.filename);
    FILE *fp;
    fp = fopen(file_path, "r");
    if(fp == NULL){
        return NO; // do not have such file
    }

    fclose(fp);
    return YES; // have such file
}

Status GETFileOpen2Server(char *username, FILE **server_fp, fileSizeType *s_filesize, \
                          struct protocolInfo *server_cmd, char *disc_base_path)
{
    struct fileInfo myfile;
    fileSizeType offset;

    GET_Cmd2fileInfo(username, &myfile, &offset, server_cmd);

    char file_path[BUF_SIZE] = {0};
    strcpy(file_path, disc_base_path);
    strcpy(file_path, myfile.filename);
    *server_fp = fopen(file_path, "r");
    if(*server_fp == NULL){
        errHandler("GETFileOpen2Server", "fopen error", NO_EXIT);
        return MYERROR; // do not have such file
    }

    *s_filesize = myfile.filesize - offset;

    fseek(*server_fp, offset, SEEK_SET);

    return OK;
}
