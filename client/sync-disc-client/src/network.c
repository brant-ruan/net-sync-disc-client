#include "client.h"
#include <winsock2.h>

/* #pragma comment(lib, "ws2_32.lib") - gcc does not need it */

const unsigned short DATA_PORT = 1500; // data port
const unsigned short CTRL_PORT = 1501; // control port
const unsigned short BEAT_PORT = 1502; // heart beat port

const unsigned short SERVER_MAIN_PORT = 10000;

const char SERVER_IP[IP_LEN + 1] = "192.168.1.110"; // by default

char UID[MD5_CHAR_LEN + 1] = {0};

/*
 * Function:
 *  use `ipconfig /all > ipconfig_tmp.dat`
 *  then MD5File(UID, ipconfig_tmp.dat)
 *
 */
Status UIDInit()
{
    system("ipconfig /all > ipconfig_tmp.dat");
    MD5File(UID, "ipconfig_tmp.dat");
    system("del ipconfig_tmp.dat");

    return OK;
}

/* initialize WSA */
Status WSAInit(WSADATA *wsaData)
{
	WORD sockVersion = MAKEWORD(2, 2);
	if (WSAStartup(sockVersion, wsaData) != 0) {
		errHandler("WSAInit", "WSAStartup Error", NO_EXIT);
		return ERROR;
	}

	return OK;
}

/*
 * Function:
 *  communicate with server and identify the user
 * Used by login.c
 */
Status Identify(char *username, char *password_md5, int username_len)
{
    UIDInit();



    return OK;
}

/*
 * Function:
 *  ask server to add an account
 * Used by signup.c
 */
Status AddUser(char *username, char *password_md5, int username_len)
{
    UIDInit();



    return OK;
}
