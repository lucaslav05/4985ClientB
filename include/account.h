//
// Created by lucas-laviolette on 1/30/25.
//

#ifndef ACCOUNT_H
#define ACCOUNT_H

#define MAX_SIZE 256

struct account
{
    char username[MAX_SIZE];
    char password[MAX_SIZE];
    int  uid;
};

#endif    // ACCOUNT_H
