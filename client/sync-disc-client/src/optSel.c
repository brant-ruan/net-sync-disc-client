#include "client.h"

/*
 * Function:
 *  ask for one valid option
 */
int optSel()
{
    int opt = MYERROR;
    SelPrompt();
    while(scanf("%d", &opt) == 0 || (opt != OP_LOGIN && opt != OP_SIGNUP && opt != OP_QUIT)){
        fflush(stdin); // to empty wrong input before
        SelPrompt();
    }

    return opt;
}
