#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <time.h>

int device;

int main(void)
{
    device = hci_open_dev(1);

    if (device < 0)
    {
        device = hci_open_dev(0);
        if (device >= 0)
        {
            printf("Using hci0\n");
        }
    }
    else
    {
        printf("Using hci1\n");
    }

    return 0;
}