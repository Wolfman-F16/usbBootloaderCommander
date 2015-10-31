/*
 cbootloader.cpp - part of flashtool for AVRUSBBoot,
 an USB bootloader for Atmel AVR controllers

 Thomas Fischl <tfischl@gmx.de>

 Creation Date..: 2006-03-18
 Last change....: 2006-06-25

 Parts are taken from the PowerSwitch project by Objective Development Software GmbH
 */

#include "cbootloader.h"

static int usbGetStringAscii(usb_dev_handle *dev, int index, int langid,
        char *buf, int buflen) {
    char buffer[256];
    int rval, i;

    if ((rval = usb_control_msg(dev, USB_ENDPOINT_IN,
            USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index,
            langid, buffer, sizeof(buffer), 1000)) < 0)
        return rval;
    if (buffer[1] != USB_DT_STRING)
        return 0;
    if ((unsigned char) buffer[0] < rval)
        rval = (unsigned char) buffer[0];
    rval /= 2;
    /* lossy conversion to ISO Latin1 */
    for (i = 1; i < rval; i++) {
        if (i > buflen) /* destination buffer overflow */
            break;
        buf[i - 1] = buffer[2 * i];
        if (buffer[2 * i + 1] != 0) /* outside of ISO Latin1 range */
            buf[i - 1] = '?';
    }
    buf[i - 1] = 0;
    return i - 1;
}

/* This project uses the free shared default VID/PID. If you want to see an
 * example device lookup where an individually reserved PID is used, see our
 * RemoteSensor reference implementation.
 */
static usb_dev_handle *findDevice(void) {
    unsigned int i;
    struct usb_bus *bus = 0;
    struct usb_device *dev = 0;
    struct usb_device_descriptor descriptor;
    usb_dev_handle *handle = 0;
    unsigned int err;
    struct usb_device **devList;
    ssize_t size;
    int retVal;
    char buffer[122];
    unsigned char busId, portId;

    fprintf(stdout, "retrieving device list...\r\n");
    size = usb_find_busses();
    fprintf(stdout, "USB busses found: %d\r\n", (int) size);

    for (i = 0; i < size; i++) {
    	bus = usb_get_busses();
        dev = bus->devices;
        if (dev == 0) {
            fprintf(stderr, "no device found. is null pointer\r\n");
            break;
        }
        busId = i;

        descriptor = dev->descriptor;
        fprintf(stdout,
                "found on bus %d for vid %04x and pid %04x ",
                busId, descriptor.idVendor, descriptor.idProduct);

        if (descriptor.idVendor == USBDEV_SHARED_VENDOR
                && descriptor.idProduct == USBDEV_SHARED_PRODUCT) {
            char string[256];
            int len;
            handle = usb_open(dev); /* we need to open the device in order to query strings */
            if (!handle) {
                fprintf(stderr, "Warning: cannot open USB device: %s\n",
                		usb_strerror());
                continue;
            }

            len = usbGetStringAscii(handle, descriptor.iManufacturer, 0x0409,
                    string, sizeof(string));
            if (len < 0) {
                fprintf(stderr,
                        "warning: cannot query manufacturer for device: %s\n",
						usb_strerror());
                goto skipDevice;
            }
            if (strcmp(string, "www.fischl.de") != 0)
                goto skipDevice;

            len = usbGetStringAscii(handle, descriptor.iProduct, 0x0409, string,
                    sizeof(string));
            if (len < 0) {
                fprintf(stderr,
                        "warning: cannot query product for device: %s\n",
						usb_strerror());
                goto skipDevice;
            }
            //  fprintf(stderr, "seen product ->%s<-\n", string);
            if (strcmp(string, "AVRUSBBoot") == 0)
                break;
            skipDevice: usb_close(handle);
            handle = NULL;
        } else {
            handle = usb_open(dev); /* we need to open the device in order to query strings */
            if (!handle) {
                fprintf(stderr, "Warning: cannot open USB device: %s\n",
                		usb_strerror());
                continue;
            }
            retVal = usb_get_string_simple(handle,
                    descriptor.iProduct, buffer,(size_t) 122);
            fprintf(stdout, " %s\r\n", buffer);
            usb_close(handle);
            handle = 0;
        }
        if (handle) {
            break;
        }
        devList++;
    }

    if (!handle)
        fprintf(stderr, "Could not find USB device www.fischl.de/AVRUSBBoot\n");
    return handle;
}

CBootloader::CBootloader() {
    fprintf(stdout, "libusb init ...\r\n");
    usb_init();
    fprintf(stdout, "libusb init complete\r\n");
    if ((usbhandle = findDevice()) == NULL) {
        fprintf(stderr,
                "Could not find USB device \"AVRUSBBoot\" with vid=0x%x pid=0x%x\n",
                USBDEV_SHARED_VENDOR, USBDEV_SHARED_PRODUCT);
        exit(1);
    }
}

CBootloader::~CBootloader() {
    usb_close(usbhandle);
    fprintf(stdout, "\r\nlibusb closed\r\n");
}

unsigned int CBootloader::getPagesize() {
    char buffer[8];
    int nBytes;

    nBytes = usb_control_msg(usbhandle,
    		USB_TYPE_VENDOR | USB_RECIP_DEVICE
                    | USB_ENDPOINT_IN, 3, 0, 0, buffer, sizeof(buffer),
            5000);

    if (nBytes != 2) {
        fprintf(stderr, "Error: wrong response size in getPageSize: %d !\n",
                nBytes);
        exit(1);
    }

    return (buffer[0] << 8) | buffer[1];
}

void CBootloader::startApplication() {
    char buffer[8];
    int nBytes;

    nBytes = usb_control_msg(usbhandle,
    		USB_TYPE_VENDOR | USB_RECIP_DEVICE
                    | USB_ENDPOINT_IN, 1, 0, 0, buffer, sizeof(buffer),
            5000);

    if (nBytes != 0) {
        fprintf(stderr,
                "Error: wrong response size in startApplication: %d !\n",
                nBytes);
        exit(1);
    }
}

void CBootloader::writePage(CPage* page) {

    unsigned int nBytes;

    nBytes = usb_control_msg(usbhandle,
    		USB_TYPE_VENDOR | USB_RECIP_DEVICE
                    | USB_ENDPOINT_OUT, 2, page->getPageaddress(), 0,
            (char*) page->getData(), page->getPagesize(), (int)5000);

    if (nBytes != page->getPagesize()) {
        fprintf(stderr, "Error: wrong byte count in writePage: %d !\n", nBytes);
        exit(1);
    }
}

