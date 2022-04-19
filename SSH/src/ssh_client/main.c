#include "ssh_client.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        help_message();
        return 0;
    }

    int i = 1;
    int bytemask = 0;

    int   port     = BROADCAST_PORT;
    char* file_log = NULL;

    while (i < argc)
    {
        if (!(strcmp(argv[i], "--help") && (strcmp(argv[i], "-h"))))
        {
            bytemask |= 0x00000001;
            break;
        }
        
        if (!(strcmp(argv[i], "--broadcast") && (strcmp(argv[i], "-b"))))
        {
            bytemask |= 0x00000002;
            i++;

            if (i < argc)
            {
                port = atoi(argv[i]);

                if (!port)
                {
                    port = BROADCAST_PORT;
                }
            }

            return broadcast_client_interface(INADDR_ANY, htons(port));
        }

        if (!(strcmp(argv[i], "--history")))
        {
            return print_ssh_history();
        }
        
    }

    return 0; 
}

int print_ssh_history()
{
    int fd = open(DEFAULT_HISTORY, O_RDONLY | O_LARGEFILE);

    if (fd == -1)
    {
        printf("Server history not found. See \'ssh_client --help\'\n");
        return ERROR_OPEN;
    }

    size_t size = file_size(fd);
    if (!size) 
    {
        close(fd);
        return ERROR_OPEN;
    }

    char* buffer = (char*) calloc(size, sizeof(char));
    if (!buffer) 
    {
        close(fd);
        return ERROR_ALLOCATE;
    }

    if (read(fd, buffer, size) != size)
    {
        close(fd);
        free(buffer);
        perror("ERROR: read():");
        return  ERROR_READ;
    }

    close(fd);

    if (write(STDOUT_FILENO, buffer, size) == -1)
    {   
        free(buffer);
        perror("ERROR: write():");
        return  ERROR_WRITE;
    }

    free(buffer);
    return 0;
}


