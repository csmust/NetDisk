#include "err_str.h"

//打印错误
void err_str(const char* str,int err)
{
    perror(str);
    exit(err);
}
