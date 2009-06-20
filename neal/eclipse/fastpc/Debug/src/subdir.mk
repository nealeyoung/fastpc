################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/get_time.cpp \
../src/main.cpp \
../src/matrix.cpp \
../src/sampler.cpp \
../src/solver.cpp \
../src/test_matrix.cpp \
../src/test_sampler.cpp 

OBJS += \
./src/get_time.o \
./src/main.o \
./src/matrix.o \
./src/sampler.o \
./src/solver.o \
./src/test_matrix.o \
./src/test_sampler.o 

CPP_DEPS += \
./src/get_time.d \
./src/main.d \
./src/matrix.d \
./src/sampler.d \
./src/solver.d \
./src/test_matrix.d \
./src/test_sampler.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O1 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


