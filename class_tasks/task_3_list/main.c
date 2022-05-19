#include "list.h"

struct string
{
    char str[1000];


    plist_t list;
};

PLIST_HEAD(my_list);

int main()
{      
    struct string m1 = {.str = "1 string"};
    struct string m2 = {.str = "2 string"};
    struct string m3 = {.str = "3 string"};
    struct string m4 = {.str = "4 string"};

    INIT_PLIST_HEAD(&m1.list);
    INIT_PLIST_HEAD(&m2.list);
    INIT_PLIST_HEAD(&m3.list);
    INIT_PLIST_HEAD(&m4.list);

    plist_add_head(&m1.list, &my_list);
    plist_add_head(&m2.list, &my_list);

    plist_t* iterator_list  = NULL;
    struct string* iterator = NULL;

    plist_for_each(iterator_list, &my_list)
    {
        printf("%s\n", container_of(iterator_list, struct string, list)->str);
    }

    plist_add_tail(&m3.list, &my_list);
    plist_add_tail(&m4.list, &my_list);

    plist_for_each_entry(iterator, &my_list, list)
    {
        printf("%s\n", iterator->str);
    }

    return 0;
}