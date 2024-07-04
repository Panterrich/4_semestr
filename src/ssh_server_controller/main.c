#include "controller.h"

int main()
{
    pid_t pid_daemon = get_daemon_pid();

    if (pid_daemon <= 0) return 1;

    if (!check_pid(pid_daemon)) return 1;

    menu(pid_daemon);

    return 0;
}
