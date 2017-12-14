################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/final_storage.c \
../src/global_reduce.c \
../src/local_reduce.c \
../src/master.c \
../src/transform.c \
../src/utils.c 

OBJS += \
./src/final_storage.o \
./src/global_reduce.o \
./src/local_reduce.o \
./src/master.o \
./src/transform.o \
./src/utils.o 

C_DEPS += \
./src/final_storage.d \
./src/global_reduce.d \
./src/local_reduce.d \
./src/master.d \
./src/transform.d \
./src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2017-2c-Deus-Vult/ipc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


