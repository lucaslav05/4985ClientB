//
// Created by lucas-laviolette on 1/30/25.
//

#ifndef ACCOUNT_H
#define ACCOUNT_H

#define MAX_USER 50
#define MAX_PASS 50

struct account
{
    char username[MAX_USER];
    char password[MAX_PASS];
    int  uid;
};

#endif    // ACCOUNT_H
