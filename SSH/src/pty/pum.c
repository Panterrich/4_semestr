#include "pum.h"

struct pam_conv conv = {misc_conv, NULL};

int login_into_user(char* username)
{
    pam_handle_t* pam = NULL;

    int ret = pam_start("power_ssh", username, &conv, &pam);
    if (ret != PAM_SUCCESS)
    {
        fprintf(stderr, "ERROR: pam_start()\n");
        return -1;
    }

    ret = pam_authenticate(pam, 0);
    if (ret != PAM_SUCCESS)
    {
        fprintf(stderr, "Incorrect password\n");
        return -1;
    }

    ret = pam_acct_mgmt(pam, 0);
    if (ret != PAM_SUCCESS)
    {
        fprintf(stderr, "User account expired!\n");
        return -1;
    }

    return 0;
}