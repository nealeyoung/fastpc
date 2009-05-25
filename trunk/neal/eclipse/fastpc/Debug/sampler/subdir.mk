################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../sampler/sampler.cc \
../sampler/test.cc 

OBJS += \
./sampler/sampler.o \
./sampler/test.o 

CC_DEPS += \
./sampler/sampler.d \
./sampler/test.d 


# Each subdirectory must supply rules for building sources it contributes
sampler/%.o: ../sampler/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


