#include <cstdio>
#include <cstring>
#include <libusb.h>
#include <cstdint>

/* Device related constants */
#define VID 0x1A86
#define PID 0x5537

#define EP_OUT 0x03 // EP3 OUT
#define EP_IN 0x84  // EP4 IN

#define TIMEOUT_MS 1000

#define USB_REQ_TYP_VENDOR 0x40

/* Protocol Constants */
#define FRAME_SOF 0x0A
#define HEADER_SIZE 5 // SOF(2) + Type(1) + Len(2)
#define FOOTER_SIZE 1 // Checksum(1)
#define OVERHEAD (HEADER_SIZE + FOOTER_SIZE)

#define MAX_PAYLOAD 1024
#define MAX_FRAME (MAX_PAYLOAD + OVERHEAD)
#define FRAME_SIZE(len) ((len) + OVERHEAD)

libusb_context *ctx = nullptr;
libusb_device_handle *handle = nullptr;
bool interfaceClaimed = false;

void usb_send_control(libusb_device_handle *handle, int cRequest, int cIndex, int cValue)
{
    int res = libusb_control_transfer(
        handle,
        USB_REQ_TYP_VENDOR, // bmRequestType: 0x40
        cRequest,           // bRequest:      0x01
        cIndex,             // wValue:        Param 1 (0=Off, 1=On)
        cValue,             // wIndex:        Param 2 (LED ID)
        NULL,               // Data Buffer:   None
        0,                  // wLength:       0
        1000                // Timeout:       1000ms
    );

    if (res < 0)
        printf("USB Cmd Failed\n");
    else
        printf("USB Cmd Sent\n");
}

void build_frame(uint8_t *frame, uint8_t type, const uint8_t *payload, uint16_t len)
{
    uint8_t *ptr = frame;
    uint8_t checksum = 0;

    *ptr++ = FRAME_SOF;
    *ptr++ = FRAME_SOF;
    *ptr++ = type;
    *ptr++ = (uint8_t)(len & 0xFF);
    *ptr++ = (uint8_t)((len >> 8) & 0xFF);

    if (len > 0 && payload != NULL)
    {
        memcpy(ptr, payload, len);
        ptr += len;
    }

    for (int i = 0; i < (ptr - frame); i++)
    {
        checksum ^= frame[i];
    }

    *ptr = checksum;
}

bool usb_send_frame(uint8_t type, const uint8_t *payload, uint16_t len)
{
    uint8_t frame[MAX_FRAME];
    build_frame(frame, type, payload, len);

    int transferred;
    return libusb_bulk_transfer(
               handle,
               EP_OUT,
               frame,
               FRAME_SIZE(len),
               &transferred,
               0) == 0;
}

int main()
{
    int result = libusb_init(&ctx);
    if (result < 0)
    {
        printf("libusb_init failed\n");
        return -1;
    }

    handle = libusb_open_device_with_vid_pid(ctx, VID, PID);
    if (!handle)
    {
        printf("Device not found\n");
        libusb_exit(ctx);
        return -1;
    }

    libusb_set_auto_detach_kernel_driver(handle, 1);

    result = libusb_claim_interface(handle, 0);
    if (result < 0)
    {
        printf("Failed to claim interface: %s\n",
               libusb_error_name(result));
    }
    else
    {
        interfaceClaimed = true;
        printf("Interface claimed\n");
    }

    if (interfaceClaimed)
    {
        while (1)
        {
            printf("Enter LED Status:\n");
            uint8_t LED_SW = 0;
            scanf("%d", &LED_SW);
            usb_send_control(handle, 0x01, 0x01, LED_SW);

            // --- ADDED: Bulk transfer test logic ---
            printf("\n--- Running Bulk Echo Test ---\n");
            uint8_t tx_buf[64];
            uint8_t rx_buf[64];
            int transferred = 0;

            for (int i = 0; i < 64; i++) { tx_buf[i] = static_cast<uint8_t>(i); }

            result = libusb_bulk_transfer(handle, EP_OUT, tx_buf, sizeof(tx_buf), &transferred, TIMEOUT_MS);
            if (result < 0) {
                printf("Bulk OUT failed: %s\n", libusb_error_name(result));
                continue; // Continue to next loop iteration
            }

            result = libusb_bulk_transfer(handle, EP_IN, rx_buf, sizeof(rx_buf), &transferred, TIMEOUT_MS);
            if (result < 0) {
                printf("Bulk IN failed: %s\n", libusb_error_name(result));
                continue; // Continue to next loop iteration
            }
            
            if (transferred == 64 && memcmp(tx_buf, rx_buf, 64) == 0) {
                printf("Data verification PASSED\n");
            } else {
                printf("Data verification FAILED (Received %d bytes)\n", transferred);
            }
        }

        // BUG FIX: The interface should be released and libusb closed only once, after the loop.
        libusb_release_interface(handle, 0);
        libusb_close(handle);
        libusb_exit(ctx);

        printf("Done\n");
    }
    return 0;
}
