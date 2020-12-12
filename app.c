#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define HARUDEV_IOCGSIZE 2147772417
#define PATH "/dev/main"
int main()
{
    int fd = open(PATH, O_RDWR);
    if (fd < 0) {
        printf("file open error!\n");
        return -1;
    }

    printf("testing ioctl...\n");
    int size = -1;
    ioctl(fd, HARUDEV_IOCGSIZE, &size);
    printf("the ioctl return size = %d\n", size);

    return close(fd)? 1 : 0;
}