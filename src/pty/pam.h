#ifndef PUM_H
#define PUM_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>

int login_into_user(char* username);

int set_id(const char* username);

#endif // PUM_H