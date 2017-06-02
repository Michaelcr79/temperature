#include <time.h>
#include <stdio.h>

int main(void)
{
    time_t current_time;
    char * buffer;
    current_time = time(NULL);
    buffer = ctime(&current_time);

    printf("%s\n",buffer);

    return 0;
}
