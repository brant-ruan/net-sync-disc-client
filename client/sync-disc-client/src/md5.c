#include "client.h"

/*
 * Function:
 *  calculate MD5 for the content of [filename]
 *  and store the characters of MD5 in [md5] ([md5] must point to
 *  an array with the length of at least MD5_CHR_LEN+1)
 */
Status MD5Gen(unsigned char *md5, char *filename)
{
    unsigned char md5_num[MD5_NUM_LEN + 1] = {0};

    FILE *fd=fopen(filename,"r");
    MD5_CTX c;
    if(fd == NULL){
        errHandler("MD5Gen", "fopen failed", NO_EXIT);
        return ERROR;
    }

    int len;
    unsigned char *pData = (unsigned char *)malloc(1024*1024*1024);

    if(pData == NULL){
        errHandler("MD5Gen", "malloc failed", NO_EXIT);
        return ERROR;
    }

    MD5_Init(&c);

    while(0 != (len = fread(pData, 1, 1024*1024*1024, fd)))
        MD5_Update(&c, pData, len);

    MD5_Final(md5,&c);

    fclose(fd);
    free(pData);

    int i;
    for(i = 0; i < MD5_NUM_LEN; i++){
        sprintf(md5[2 * i], "%02x", md5_num[i]);
    }

    return OK;
}
