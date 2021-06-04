#include <uuid/uuid.h>

// return: != 16失败
int get_uuid(char *uuid, size_t uuid_size)
{
    uuid_t _uuid;
    uuid_generate(_uuid);
    unsigned char *p = _uuid;
    int i;
    char ch[5] = {0};
    for (i = 0; i < sizeof(uuid_t); i++, p++)
    {
        snprintf(ch, sizeof(ch), "%02X", *p);
        uuid[i * 2] = ch[0];
        uuid[i * 2 + 1] = ch[1];
    }
    return i;
}