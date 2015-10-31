/*
 cbootloader.cpp - part of flashtool for AVRUSBBoot, an USB bootloader for Atmel AVR controllers

 Thomas Fischl <tfischl@gmx.de>

 Creation Date..: 2006-03-18
 Last change....: 2006-06-25

 Parts are taken from the PowerSwitch project by Objective Development Software GmbH
 */

#include "cbootloader.h"

static libusb_context *ctx;

static int usbGetStringAscii(libusb_device_handle *dev, int index, int langid,
        char *buf, int buflen) {
    unsigned char buffer[256];
    int rval, i;

    if ((rval = libusb_control_transfer(dev, LIBUSB_ENDPOINT_IN,
            LIBUSB_REQUEST_GET_DESCRIPTOR, (LIBUSB_DT_STRING << 8) + index,
            langid, buffer, sizeof(buffer), 1000)) < 0)
        return rval;
    if (buffer[1] != LIBUSB_DT_STRING)
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
static libusb_device_handle *findDevice(void) {
    unsigned int i;
    struct libusb_device *dev = 0;
    struct libusb_device_descriptor descriptor;
    libusb_device_handle *handle = 0;
    libusb_error err;
    struct libusb_device **devList;
    ssize_t size;
    int retVal;
    unsigned char buffer[122];
    uint8_t busId, portId;
    uint8_t path[8];

    fprintf(stdout, "retrieving device list...\r\n");
    size = libusb_get_device_list(NULL, &devList);
    fprintf(stdout, "USB devices found: %d\r\n", (int) size);

    for (i = 0; i < size; i++) {
        dev = *devList;
        if (dev == 0) {
            fprintf(stderr, "no device found. is nullpointer\r\n");
            break;
        }

        retVal = libusb_get_device_descriptor(dev, &descriptor);
        if(retVal < 0) {
        	return 0;
        }
        busId = libusb_get_bus_number(dev);
        portId = libusb_get_port_number(dev);
        fprintf(stdout, "%04x:%04x (bus %d, device %d) path %d\r\n",
                			descriptor.idVendor, descriptor.idProduct,
                			busId, libusb_get_device_address(dev), portId);

        if (descriptor.idVendor == USBDEV_SHARED_VENDOR
                && descriptor.idProduct == USBDEV_SHARED_PRODUCT) {
            char string[256];
            int len;
            err = (libusb_error) libusb_open(dev, &handle); /* we need to open the device in order to query strings */
            if (!handle) {
                fprintf(stderr, "Warning: cannot open USB device: %s\n",
                        libusb_strerror(err));
                continue;
            }

            len = usbGetStringAscii(handle, descriptor.iManufacturer, 0x0409,
                    string, sizeof(string));
            if (len < 0) {
                fprintf(stderr,
                        "warning: cannot query manufacturer for device: %s\n",
                        libusb_strerror(LIBUSB_ERROR_NOT_FOUND));
                goto skipDevice;
            }
            if (strcmp(string, "www.fischl.de") != 0)
                goto skipDevice;

            int LIBUSB_CALL libusb_get_string_descriptor_ascii(
                    libusb_device_handle *dev, uint8_t desc_index,
                    unsigned char *data, int length);

            len = usbGetStringAscii(handle, descriptor.iProduct, 0x0409, string,
                    sizeof(string));
            if (len < 0) {
                fprintf(stderr,
                        "warning: cannot query product for device: %s\n",
                        libusb_strerror(LIBUSB_ERROR_NOT_FOUND));
                goto skipDevice;
            }
            //  fprintf(stderr, "seen product ->%s<-\n", string);
            if (strcmp(string, "AVRUSBBoot") == 0)
                break;
            skipDevice: libusb_close(handle);
            handle = NULL;
        } else {
            /* do nothing */
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
	ctx = NULL;
    fprintf(stdout, "libusb init ...\r\n");
    libusb_init(NULL);
    fprintf(stdout, "libusb init complete with context %d\r\n",
            (unsigned long int) ctx);
    if ((usbhandle = findDevice()) == NULL) {
        fprintf(stderr,
                "Could not find USB device \"AVRUSBBoot\" with vid=0x%x pid=0x%x\n",
                USBDEV_SHARED_VENDOR, USBDEV_SHARED_PRODUCT);
        exit(1);
    }
}

CBootloader::~CBootloader() {
    libusb_close(usbhandle);
    fprintf(stdout, "\r\nlibusb closed\r\n");
}

unsigned int CBootloader::getPagesize() {
    unsigned char buffer[8];
    int nBytes;

    nBytes = libusb_control_transfer(usbhandle,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
                    | LIBUSB_ENDPOINT_IN, 3, 0, 0, buffer, sizeof(buffer),
            5000);

    if (nBytes != 2) {
        fprintf(stderr, "Error: wrong response size in getPageSize: %d !\n",
                nBytes);
        exit(1);
    }

    return (buffer[0] << 8) | buffer[1];
}

void CBootloader::startApplication() {
    unsigned char buffer[8];
    int nBytes;

    nBytes = libusb_control_transfer(usbhandle,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
                    | LIBUSB_ENDPOINT_IN, 1, 0, 0, buffer, sizeof(buffer),
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

    nBytes = libusb_control_transfer(usbhandle,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
                    | LIBUSB_ENDPOINT_OUT, 2, page->getPageaddress(), 0,
            (unsigned char*) page->getData(), page->getPagesize(), 5000);

    if (nBytes != page->getPagesize()) {
        fprintf(stderr, "Error: wrong byte count in writePage: %d !\n", nBytes);
        exit(1);
    }
}

