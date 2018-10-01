# AVR-GCC Makefile
TRG=vescuartapi
# type of build, values: LINUX, AVR, ARDUINO
BUILDTYPE=LINUX

# only for ARDUINO and AVR
MMCU=atmega2560
MMCUDRAGON=m2560
# only for AVR, MCU speed, usually 16 MHz for 5 V versions, 8MHz for 3.3 V version
F_CPU = 16000000





SOURCES=../src/buffer.cpp ../src/crc.cpp ../common/ringbuffer.cpp ../src/vescuartapi.cpp
OBJ=buffer.o crc.o ringbuffer.o vescuartapi.o

RM	= rm -f
RN	= mv
CP	= cp

ifeq ($(BUILDTYPE), LINUX)
  CC	= gcc
  CPP	= g++
  AS	= gcc -x assembler-with-cpp	
  OBJCOPY	= objcopy
  SIZE	= size
  CPFLAGS = -O2 -Wall -Wextra -DLINUXBUILD -ggdb3 -fno-exceptions -std=c++11
  SOURCES += linux_hwserial.cpp example_linux.cpp
  OBJ += linux_hwserial.o example_linux.o
  GOAL = $(TRG)_linux
endif
ifeq ($(BUILDTYPE), AVR)
  CC	= avr-gcc
  CPP	= avr-g++
  AS	= avr-gcc -x assembler-with-cpp	
  OBJCOPY	= avr-objcopy
  SIZE	= avr-size
  LDFLAGS = -mmcu=$(MMCU)
  FORMAT = ihex	
  CPFLAGS = -mmcu=$(MMCU) -Os -Wall -Wextra -fno-exceptions -DAVRBUILD -DF_CPU=$(F_CPU)
  SOURCES += avr_hwserial.cpp example_avr.cpp
  OBJ += avr_hwserial.o example_avr.o
  GOAL = $(TRG).elf $(TRG).bin $(TRG).hex $(TRG).eep $(TRG).ok program
endif

ifeq ($(BUILDTYPE), ARDUINO)
  CC	= avr-gcc
  CPP	= avr-g++
  AS	= avr-gcc -x assembler-with-cpp	
  OBJCOPY	= avr-objcopy
  SIZE	= avr-size
  LDFLAGS = -mmcu=$(MMCU)
  FORMAT = ihex	
  CPFLAGS = -mmcu=$(MMCU) -Os -Wall -DAVRBUILD
endif

INCLUDES=-I.. -I. -I../common -I../src

all:	$(GOAL)

program:	$(TRG).hex
	avrdude -p $(MMCUDRAGON) -c arduino -P /dev/ttyUSB0 -b 57600 -e -U flash:w:$(TRG).hex

%.o : %.c 
	$(CC) -c $(CPFLAGS) $(INCLUDES) $< -o $@

%.o : %.cpp
	$(CPP) -c $(CPFLAGS) $(INCLUDES) $< -o $@

%.o : ../%.cpp
	$(CPP) -c $(CPFLAGS) $(INCLUDES) $< -o $@

%.o : ../common/%.cpp
	$(CPP) -c $(CPFLAGS) $(INCLUDES) $< -o $@

%.o : ../src/%.cpp
	$(CPP) -c $(CPFLAGS) $(INCLUDES) $< -o $@

%.o : %.S
	$(AS) -c $(CPFLAGS) $(INCLUDES) $< -o $@

%.s : %.c
	$(CC) -S $(CPFLAGS) $(INCLUDES) $< -o $@

%.s : %.cpp
	$(CPP) -S $(CPFLAGS) $(INCLUDES) $< -o $@

vescuartapi_linux: $(OBJ)
	$(CC) $(OBJ) $(CPFLAGS) $(LIB) $(LDFLAGS) -o $@

%.elf: $(OBJ)
	$(CC) $(OBJ) $(LIB) $(LDFLAGS) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary -R .eeprom $< $@

%.hex: %.elf
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

%.eep: %.elf
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O $(FORMAT) $< $@

%ok:
	$(SIZE) -C --mcu=$(MMCU) $(TRG).elf
	@echo "Errors: none" 

clean:
	$(RM) $(OBJ)
	$(RM) $(TRG).map
	$(RM) $(TRG).elf
	$(RM) $(TRG).cof
	$(RM) $(TRG).obj
	$(RM) $(TRG).a90
	$(RM) $(TRG).sym
	$(RM) $(TRG).eep
	$(RM) $(TRG).hex
	$(RM) $(TRG).bin
	$(RM) *~
	@echo "Errors: none"
	
size:
	$(SIZE) $(TRG).elf
	
