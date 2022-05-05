#include "pam.h"
#include "syslog.h"

struct pam_conv conv = {misc_conv, NULL};

int login_into_user(char* username)
{
    pam_handle_t* pam = NULL;

    int ret = pam_start("power_ssh", username, &conv, &pam);
    if (ret != PAM_SUCCESS)
    {
        syslog(LOG_INFO, "[PAM] pam_start \"%s\" user, errno = %d: %s", username, errno, pam_strerror(pam, ret));
        fprintf(stderr, "ERROR: pam_start()\n");
        return -1;
    }

    syslog(LOG_INFO, "[RAM] pam_start after \"%s\" user, errno = %d: %s", username, errno, pam_strerror(pam, ret));


    ret = pam_authenticate(pam, 0);
    if (ret != PAM_SUCCESS)
    {
        syslog(LOG_INFO, "[PAM] authenticate \"%s\" user, errno = %d: %s", username, errno, pam_strerror(pam, ret));
        fprintf(stderr, "Incorrect password\n");
        return -1;
    }

    syslog(LOG_INFO, "[PAM] authenticate \"%s\" user, errno = %d: %s", username, errno, pam_strerror(pam, ret));


    ret = pam_acct_mgmt(pam, 0);
    if (ret != PAM_SUCCESS)
    {
        syslog(LOG_INFO, "[PAM] pacm_acct_mgmt \"%s\" user, errno = %d: %s", username, errno, pam_strerror(pam, ret));
        fprintf(stderr, "User account expired!\n");
        return -1;
    }

    syslog(LOG_INFO, "[PAM] pacm_acct_mgmt after \"%s\" user, errno = %d: %s", username, errno, pam_strerror(pam, ret));
    errno = 0;
    return 0;
}

int set_id(const char* username)
{
    struct passwd* info = getpwnam(username);
    if (!info)
    {
        perror("getpwnam()");
        return -1;
    }

    if (setgid(info->pw_gid) == -1)
    {
        perror("setgid()");
        return -1;
    }

    if (setuid(info->pw_uid) == -1)
    {
        perror("setuid()");
        return -1;
    }

    return 0;
}