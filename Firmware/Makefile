# Makefile for PlatformIO Project

# Command to execute PlatformIO
# You can override this by running `make PIO=/path/to/pio`
PIO ?= pio

# Default environment
# ENV ?= seeed_xiao_esp32s3

PIO_ARGS =
ifdef ENV
	PIO_ARGS += -e $(ENV)
endif

# Default target
all: build

# Build the project
build:
	$(PIO) run $(PIO_ARGS)

# Upload firmware to the device
upload:
	$(PIO) run --target upload $(PIO_ARGS)

# Clean build artifacts
clean:
	$(PIO) run --target clean $(PIO_ARGS)

# Open serial monitor
monitor:
	$(PIO) device monitor $(PIO_ARGS)

# Run menuconfig (if supported by platform/framework)
menuconfig:
	$(PIO) run --target menuconfig $(PIO_ARGS)

# Upload filesystem image (LittleFS/SPIFFS)
uploadfs:
	$(PIO) run --target uploadfs $(PIO_ARGS)

# Update project libraries
update:
	$(PIO) pkg update

# List devices
devices:
	$(PIO) device list

.PHONY: all build upload clean monitor menuconfig update devices
