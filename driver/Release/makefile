################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
LSS += \
ampera-two.lss \

FLASH_IMAGE += \
ampera-two.hex \

EEPROM_IMAGE += \
ampera-two.eep \

SIZEDUMMY += \
sizedummy \


# All Target
all: ampera-two.elf secondary-outputs

# Tool invocations
ampera-two.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	avr-gcc -Wl,-Map,ampera-two.map -Wl,-u,vfprintf -mmcu=atmega328p -o "ampera-two.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

ampera-two.lss: ampera-two.elf
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S ampera-two.elf  >"ampera-two.lss"
	@echo 'Finished building: $@'
	@echo ' '

ampera-two.hex: ampera-two.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -R .fuse -R .lock -R .signature -O ihex ampera-two.elf  "ampera-two.hex"
	@echo 'Finished building: $@'
	@echo ' '

ampera-two.eep: ampera-two.elf
	@echo 'Create eeprom image (ihex format)'
	-avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex ampera-two.elf  "ampera-two.eep"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: ampera-two.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=atmega328p ampera-two.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(FLASH_IMAGE)$(ELFS)$(OBJS)$(ASM_DEPS)$(EEPROM_IMAGE)$(S_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS)$(LSS)$(C_DEPS) ampera-two.elf
	-@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(EEPROM_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents

-include ../makefile.targets
