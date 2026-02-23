#include <cstdio>
#include <cstring>
#include "libusb.h"
#include <cstdint>
#include <cstdlib> // For atof

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

// Custom message type for combined ticker and price
#define MSG_TYPE_TICKER_AND_PRICE 0x03

// Define fixed size for ticker string within the combined payload
#define TICKER_FIXED_LEN 8
#define COMBINED_PAYLOAD_LEN (TICKER_FIXED_LEN + sizeof(float))

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
        fprintf(stderr, "USB Cmd Failed\n");
    else
        fprintf(stderr, "USB Cmd Sent\n");
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
               TIMEOUT_MS) == 0;
}

int main(int argc, char *argv[])
{
    int result = libusb_init(&ctx);
    if (result < 0)
    {
        fprintf(stderr, "libusb_init failed\n");
        return -1;
    }

    handle = libusb_open_device_with_vid_pid(ctx, VID, PID);
    if (!handle)
    {
        fprintf(stderr, "Device not found\n");
        libusb_exit(ctx);
        return -1;
    }

    libusb_set_auto_detach_kernel_driver(handle, 1);

    result = libusb_claim_interface(handle, 0);
    if (result < 0)
    {
        fprintf(stderr, "Failed to claim interface: %s\n",
               libusb_error_name(result));
        libusb_close(handle);
        libusb_exit(ctx);
        return -1;
    }
    else
    {
        interfaceClaimed = true;
        fprintf(stderr, "Interface claimed\n");
    }

    if (interfaceClaimed)
    {
        char line_buf[MAX_PAYLOAD];
        uint8_t combined_payload[COMBINED_PAYLOAD_LEN];

        while (fgets(line_buf, sizeof(line_buf), stdin) != NULL)
        {
            // Remove newline character if present
            line_buf[strcspn(line_buf, "\n")] = 0;
            fprintf(stderr, "Received from stdin: %s\n", line_buf);

            char *ticker_start = line_buf;
            char *price_str_start = NULL;
            float price_value = 0.0f; // Default price

            // Find the delimiter ": "
            char *delimiter_pos = strstr(line_buf, ": ");
            if (delimiter_pos != NULL)
            {
                // Extract ticker
                int ticker_len = delimiter_pos - ticker_start;
                if (ticker_len > TICKER_FIXED_LEN) {
                    ticker_len = TICKER_FIXED_LEN; // Truncate if too long
                }
                memcpy(combined_payload, ticker_start, ticker_len);
                // Pad with nulls if ticker is shorter than TICKER_FIXED_LEN
                memset(combined_payload + ticker_len, '\0', TICKER_FIXED_LEN - ticker_len);
                
                // Extract price string and convert to float
                price_str_start = delimiter_pos + 2; // Move past ": "
                if (price_str_start != NULL && *price_str_start != '\0')
                {
                    price_value = atof(price_str_start);
                }
            }
            else
            {
                // If no delimiter, treat the whole line as ticker and set price to 0
                int ticker_len = strlen(ticker_start);
                if (ticker_len > TICKER_FIXED_LEN) {
                    ticker_len = TICKER_FIXED_LEN; // Truncate
                }
                memcpy(combined_payload, ticker_start, ticker_len);
                memset(combined_payload + ticker_len, '\0', TICKER_FIXED_LEN - ticker_len);
            }

            // Copy the float value directly into the payload after the ticker
            memcpy(combined_payload + TICKER_FIXED_LEN, &price_value, sizeof(float));

            // Send the combined payload
            if (usb_send_frame(MSG_TYPE_TICKER_AND_PRICE, combined_payload, COMBINED_PAYLOAD_LEN))
            {
                fprintf(stderr, "Combined ticker and price sent successfully.\n");
            }
            else
            {
                fprintf(stderr, "Failed to send combined ticker and price.\n");
            }
        }

        libusb_release_interface(handle, 0);
        libusb_close(handle);
        libusb_exit(ctx);

        fprintf(stderr, "Done\n");
    }
    return 0;
}
