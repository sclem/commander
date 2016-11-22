#include <stdlib.h>

char *getsetenv(const char *envname, char *default_value) {
    char *value = getenv(envname);
    if (!value) {
        value = default_value;
    }
    return value;
}
