#include "client.h"


void Prompt()
{
    printf("\n+---------------------------------------------------------------+");
    printf("\n+                         Net Sync Disc                         +");
    printf("\n+---------------------------------------------------------------+");
    printf("\n+                                                               +");
    printf("\n+                         %d <-> Log in                          +", OP_LOGIN);
    printf("\n+                         %d <-> Sign up                         +", OP_SIGNUP);
    printf("\n+                         %d <-> Quit                            +", OP_QUIT);
    printf("\n+                                                               +");
    printf("\n+---------------------------------------------------------------+");
    printf("\n+");
    printf("\n+-------> Option: ");
}

/*
 * Function:
 *  ask for one valid option
 */
int optSel()
{
    int opt = ERROR;
    Prompt();
    while(scanf("%d", &opt) == 0 || (opt != OP_LOGIN && opt != OP_SIGNUP && opt != OP_QUIT)){
        system("cls");
        fflush(stdin); // to empty wrong input before
        Prompt();
    }

    return opt;
}
