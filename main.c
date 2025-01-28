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
#include <unistd.h>

int device_id = 0;
int sock = 0;

static void update_device_id(void)
{
    device_id = hci_get_route(NULL);
}

static int get_device_id(void)
{
    return device_id;
}

static int get_sock(void)
{
    return sock;
}

static void scan_ble_devices(int dev_id)
{
    sock = hci_open_dev(dev_id);

    if (dev_id < 0 || sock < 0)
    {
        perror("opening sock");
        exit(1);
    }
    inquiry_info *ii = NULL;

    int len, flags;
    int max_rsp, num_rsp;

    char addr[19] = {0};
    char name[248] = {0};

    len = 8;
    max_rsp = 255;
    flags = IREQ_CACHE_FLUSH;

    ii = (inquiry_info *)malloc(max_rsp * sizeof(inquiry_info));

    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);

    if (num_rsp < 0)
        perror("hci_inqury");

    for (int i = 0; i < num_rsp; i++)
    {
        ba2str(&(ii + i)->bdaddr, addr);

        memset(name, 0, sizeof(name));

        if (hci_read_remote_name(sock, &(ii + i)->bdaddr, sizeof(name), name, 0) < 0)
        {
            strcpy(name, "[UNKNOWN]");
        }
        printf("%s %s\n", addr, name);
    }

    free(ii);
    close(sock);
}

int main(void)
{
    update_device_id();

    while (1)
    {
        scan_ble_devices(get_device_id());
        sleep(5);
    }

    return 0;
}