#include "client.h"

/* This file defines all the prompt functions */

void SignupPrompt()
{
    printf("\n+---------------------------------------------------------------+");
    printf("\n+                    Net Sync Disc - Sign up                    +");
    printf("\n+---------------------------------------------------------------+");
    printf("\n+                                                                ");
    printf("\n+");
}

void LoginPrompt()
{
    printf("\n+---------------------------------------------------------------+");
    printf("\n+                    Net Sync Disc - Log in                     +");
    printf("\n+---------------------------------------------------------------+");
    printf("\n+                                                                ");
    printf("\n+");
}

void SelPrompt()
{
    system("cls");
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

void DiscDirPrompt(char* username)
{
    system("cls");
    printf("\n+---------------------------------------------------------------+");
    printf("\n+                         Net Sync Disc                         +");
    printf("\n+---------------------------------------------------------------+");
    printf("\n+-------> Welcome, %s :)\n", username);
}
