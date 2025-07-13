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

#define SCAN_INTERVAL 0x0010 // Scan interval (0.625ms units, 0x0010 = 16ms)
#define SCAN_WINDOW 0x0010   // Scan window (0.625ms units, 0x0010 = 16ms)
#define MAX_RESPONSES 255    // Maximum number of devices to discover

int device_id = 0;
int sock = 0;

static void update_device_id(void)
{
    device_id = hci_get_route(NULL);
    printf("device_id: %d\n", device_id);
}

static int get_device_id(void)
{
    return device_id;
}

static void scan_ble_devices(int dev_id)
{
    sock = hci_open_dev(dev_id);
    printf("sock: %d\n", sock);
    if (dev_id < 0 || sock < 0)
    {
        perror("opening sock");
        exit(1);
    }

    struct hci_request req;
    memset(&req, 0, sizeof(req));
    req.ogf = OGF_LE_CTL;
    req.ocf = OCF_LE_SET_SCAN_PARAMETERS;

    uint8_t scan_params[5] = {0x01, SCAN_INTERVAL & 0xFF, SCAN_INTERVAL >> 8, SCAN_WINDOW & 0xFF, SCAN_WINDOW >> 8};
    req.cparam = scan_params;
    req.clen = sizeof(scan_params);

    if (hci_send_req(sock, &req, 1000) < 0)
    {
        perror("Failed to set scan parameter");
        close(sock);
        return;
    }

    memset(&req, 0, sizeof(req));
    req.ogf = OGF_LE_CTL;
    req.ocf = OCF_LE_SET_SCAN_ENABLE;
    uint8_t scan_enable[2] = {0x01, 0x00};
    req.cparam = scan_enable;
    req.clen = sizeof(scan_enable);
    if (hci_send_req(sock, &req, 1000) < 0)
    {
        perror("Failed to enable scan");
        close(sock);
        return;
    }

    uint8_t buffer[HCI_MAX_EVENT_SIZE];

    while (1)
    {
        printf("Waiting for events...\n");
        int len = read(sock, buffer, sizeof(buffer));
        printf("Read %d bytes from socket\n", len);
        if (len < 0)
        {
            perror("Failed to read from socket");
            close(sock);
            return;
        }

        if (buffer[0] == EVT_LE_META_EVENT)
        {
            evt_le_meta_event *meta_event = (evt_le_meta_event *)(buffer + 1);
            if (meta_event->subevent == EVT_LE_ADVERTISING_REPORT)
            {
                le_advertising_info *info = (le_advertising_info *)(meta_event->data + 1);
                int num_reports = meta_event->data[0];

                for (int i = 0; i < num_reports; i++)
                {
                    char addr[19];
                    ba2str(&info[i].bdaddr, addr);
                    printf("Device found: %s\n", addr);
                    printf("RSSI: %d\n", info[i].data[info[i].length - 1]);
                    printf("Advertising Data: ");
                    for (int j = 0; j < info[i].length - 2; j++)
                    {
                        printf("%02x ", info[i].data[j]);
                    }
                    printf("\n");
                }
            }
        }
        usleep(100000); // Sleep for 100ms to avoid flooding the output
    }
    close(sock);
}

int main(void)
{
    update_device_id();

    while (1)
    {
        scan_ble_devices(get_device_id());
    }

    return 0;
}