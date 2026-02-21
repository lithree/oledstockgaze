################################################################################
# MRS Version: 1.9.2
# ?????????????
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../USB/ch32v30x_usbhs_device.c \
../USB/usb_desc.c 

OBJS += \
./USB/ch32v30x_usbhs_device.o \
./USB/usb_desc.o 

C_DEPS += \
./USB/ch32v30x_usbhs_device.d \
./USB/usb_desc.d 


# Each subdirectory must supply rules for building sources it contributes
USB/%.o: ../USB/%.c
	@	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized  -g -I"C:\Users\skyli\Desktop\Analog_Toolkit\Empty_proj\SPI_LCD\SRC\Debug" -I"C:\Users\skyli\Desktop\Analog_Toolkit\Empty_proj\SPI_LCD\LCD" -I"C:\Users\skyli\Desktop\Analog_Toolkit\Empty_proj\SPI_LCD\SRC\Core" -I"C:\Users\skyli\Desktop\Analog_Toolkit\Empty_proj\SPI_LCD\User" -I"C:\Users\skyli\Desktop\Analog_Toolkit\Empty_proj\SPI_LCD\SRC\Peripheral\inc" -I"C:\Users\skyli\Desktop\Analog_Toolkit\Empty_proj\SPI_LCD\USB" -I"/SPI_LCD/SRC" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

