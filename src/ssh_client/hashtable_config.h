typedef char* hkey_t;
typedef struct sockaddr_in hvalue_t;
const hkey_t   key_poison = NULL;
const hvalue_t value_poison = {-1, -1, {-1}, {-1}};

void value_dtor(hvalue_t* value)
{

}

void key_dtor(hkey_t* value)
{

}

int key_cmp(const hkey_t* left, const hkey_t* right)
{   
    if (*right == key_poison)
        return !(*left == key_poison);

    return strcmp(*left, *right);
}

void print_key_value(FILE* file, hkey_t key, hvalue_t value)
{   
    fprintf(file, "%s | %s", key, inet_ntoa(value.sin_addr));
}

