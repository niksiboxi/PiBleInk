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
#define COMPLETE_LOCAL_NAME 0x09
#define SHORT_LOCAL_NAME 0x08

int device_id = 0;
int device = 0;

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
    device = hci_open_dev(dev_id);
    printf("device: %d\n", device);
    if (dev_id < 0 || device < 0)
    {
        perror("opening device");
        exit(1);
    }

    struct hci_request req;
    memset(&req, 0, sizeof(req));
    req.ogf = OGF_LE_CTL;
    req.ocf = OCF_LE_SET_SCAN_PARAMETERS;

    uint8_t scan_params[5] = {0x01, SCAN_INTERVAL & 0xFF, SCAN_INTERVAL >> 8, SCAN_WINDOW & 0xFF, SCAN_WINDOW >> 8};
    req.cparam = scan_params;
    req.clen = sizeof(scan_params);

    if (hci_send_req(device, &req, 1000) < 0)
    {
        perror("Failed to set scan parameter");
        close(device);
        return;
    }

    memset(&req, 0, sizeof(req));
    req.ogf = OGF_LE_CTL;
    req.ocf = OCF_LE_SET_SCAN_ENABLE;
    uint8_t scan_enable[2] = {0x01, 0x00};
    req.cparam = scan_enable;
    req.clen = sizeof(scan_enable);
    if (hci_send_req(device, &req, 1000) < 0)
    {
        perror("Failed to enable scan");
        close(device);
        return;
    }

    // Get Results.
	struct hci_filter nf;
	hci_filter_clear(&nf);
	hci_filter_all_ptypes(&nf);
	hci_filter_all_events(&nf);
	if (setsockopt(device, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0)
	{
		hci_close_dev(device);
		perror("Could not set socket options\n");
		return;
	}

    uint8_t buffer[HCI_MAX_EVENT_SIZE];

    while (1)
    {
        int len = read(device, buffer, sizeof(buffer));
        if (len < 0)
        {
            perror("Failed to read from device");
            close(device);
            return;
        } 
        else
        {
            evt_le_meta_event *meta_event = (evt_le_meta_event *)(buffer + 1);
            if (meta_event->subevent == EVT_LE_META_EVENT)
            {
                le_advertising_info *info = (le_advertising_info *)(meta_event->data + 1);
                int num_reports = meta_event->data[0];

                for (int i = 0; i < num_reports; i++)
                {
                    char addr[EVT_LE_CONN_COMPLETE_SIZE];
                    ba2str(&info[i].bdaddr, addr);
                    printf("Device found: %s ", addr);

                    for (int j = 0; j < info[i].length - 2; j++)
                    {
                        uint8_t ad_type = info->data[j + 1];
                        if (ad_type == COMPLETE_LOCAL_NAME || ad_type == SHORT_LOCAL_NAME) {
                            char name[256] = {0};
                            memcpy(name, &info->data[j + 2], info->data[j - 1]);
                            printf("[%s]", name);
                        }
                    }
                    printf("\n");
                }
            }
        }
        usleep(100000); // Sleep for 100ms to avoid flooding the output
    }
    close(device);
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