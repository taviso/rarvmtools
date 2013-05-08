#include <string.h>
#include "strchrnul.h"

char *strchrnul(const char *s, int c)
{
       char *ptr = strchr(s, c);
       if (!ptr) {
               ptr = strchr(s, '\0');
       }
       return ptr;
}
