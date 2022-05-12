#include "ssh_client.h"

int main(int argc, char* argv[])
{   
    if (argc < 2)
    {
        help_message();
        return 0;
    }

    int i = 1;
    unsigned int bytemask = 0;

    in_port_t broad_port      = BROADCAST_PORT;
    char* file_log            = NULL;
    in_addr_t addr            = 0;

    char username[MAX_INPUT] = "";
    char address [MAX_INPUT] = "";
    char path_src[MAX_INPUT] = "";
    char path_dst[MAX_INPUT] = "";

    while (i < argc)
    {
        if (!(strcmp(argv[i], "--help") && (strcmp(argv[i], "-h"))))
        {
            bytemask |= 0x00000001;
            i++;
            continue;
        }
        
        if (!(strcmp(argv[i], "--broadcast") && (strcmp(argv[i], "-b"))))
        {
            bytemask |= 0x00000002;
            i++;

            if (i < argc)
            {
                broad_port = atoi(argv[i]);

                if (!broad_port)
                {
                    broad_port = BROADCAST_PORT;
                }
            }

            continue;
        }

        if (!(strcmp(argv[i], "--history")))
        {
            bytemask |= 0x00000004;
            i++;
            continue;
        }

        if (!(strcmp(argv[i], "--ssh") && strcmp(argv[i], "-s")))
        {
            i++;
            
            if (i < argc)
            {
                if (!(strcmp(argv[i], "--tcp") && strcmp(argv[i], "-t")))
                {
                    bytemask |= 0x0000010;
                    i++;
                } 
                else if (!(strcmp(argv[i], "--udp") && strcmp(argv[i], "-u")))
                {
                    bytemask |= 0x0000020;
                    i++;
                }
                else 
                {
                    bytemask |= 0x0000010;
                }
            }

            if (i < argc)
            {
                char* address = strchr(argv[i], '@');
                if (address == NULL)
                {
                    help_message();
                    return -1;
                }
                address++;         
                
                addr = inet_addr(address);
                if (addr == INADDR_NONE)
                {
                    help_message();
                    return -1;
                }

                strncpy(username, argv[i], address - 1 - argv[i]);
                i++;
            }
            else
            {   
                help_message();
                return -1;
            }

            continue;
        }

        if (!(strcmp(argv[i], "--copy") && strcmp(argv[i], "-c")))
        {
            bytemask |= 0x00000040;
            i++; 

            if (i >= argc) 
            {
                help_message();
                return 1;
            }

            strncpy(path_src, argv[i], MAX_INPUT);
            if (!check_file(path_src))
            {
                return -1;
            }

            i++;

            if (i < argc)
            {
                char* ptr_addr = strchr(argv[i], '@');
                if (ptr_addr == NULL)
                {
                    help_message();
                    return -1;
                }
                
                ptr_addr++;         
                
                char* ptr_path = strchr(argv[i], ':');
                if (ptr_path == NULL)
                {
                    help_message();
                    return -1;
                }

                ptr_path++;

                strncpy(username, argv[i],  ptr_addr - 1 - argv[i]);
                strncpy(address,  ptr_addr, ptr_path - 1 - ptr_addr);
                strncpy(path_dst, ptr_path, strlen(ptr_path));
                
                addr = inet_addr(address);
                if (addr == INADDR_NONE)
                {
                    help_message();
                    return -1;
                }

                i++;
            }
            else
            {   
                help_message();
                return -1;
            }

            continue;
        }

        help_message();
        return -1;
    }

    int error = 0;
    unsigned int comand = bytemask;

    while (bytemask)
    {
        error += (bytemask & 1);
        bytemask >>= 1;
    }

    if (error > 1) 
    {
        help_message();
        return -1;
    }

    switch (comand)
    {
        case 0x00000001:
        {
            help_message();
            break;
        }

        case 0x00000002:
        {
            return broadcast_client_interface(htonl(INADDR_ANY), htons(broad_port));
            break;

        }

        case 0x00000004:
        {
            return print_ssh_history();
            break;
        }

        case 0x00000010:
        {
            return ssh_client(addr, username, SOCK_STREAM);
            break;
        }

        case 0x00000020:
        {
            return ssh_client(addr, username, SOCK_DGRAM);
            break;
        }

        case 0x00000040:
        {
            
            return ssh_copy_file(addr, username, path_src, path_dst);
            break;
        }

        default:
        {
            help_message();
            break;
        }
    }

    return 0; 
}

