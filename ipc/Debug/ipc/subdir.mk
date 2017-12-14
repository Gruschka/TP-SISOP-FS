################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ipc/ipc.c \
../ipc/serialization.c 

OBJS += \
./ipc/ipc.o \
./ipc/serialization.o 

C_DEPS += \
./ipc/ipc.d \
./ipc/serialization.d 


# Each subdirectory must supply rules for building sources it contributes
ipc/%.o: ../ipc/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


