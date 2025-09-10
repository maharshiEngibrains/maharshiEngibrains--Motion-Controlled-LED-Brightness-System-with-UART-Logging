#This file cofngures the MCU Boot Image header/signature

#MCUboot header size
HEADER_SIZE?=0x400

#The value, which is read back from erased flash. Default: 0
ERASED_VAL?=0xff

#The user application file
INPUT_IMAGE?=$(COMPILED_HEX).hex

# Output application image
OUTPUT_IMAGE?=$(COMPILED_HEX).hex

# Slot size is set to 512KB. Calcuated as (512*1024)Bytes 
# EdgePtotect Tools assumes the slot size as 128KB by default. 
# If you don't pass this argument, default slot size will be used.
SLOT_SIZE?=0x80000

#Setting up the Additional Arguments
ADDITIONAL_ARGS=

#Start offset of the secure application in flash
#Note: this address should be same as defined in the "extended_boot_policy"
#Using Non Secure aliased address for programming Ext flash.
# Secured Alias of this address is 0x70100000
APP_START=0x60100000

ifeq ($(EDGE_PROTECT_BOOTLOADER),TRUE)

#Software component name 
BOOT_RECORD?=proj_cm33_s

#alignment of the flash device ;default 8
FLASH_ALIGNMENT?=1

#Update type can be 'overwrite' or 'swap'
UPDATE_TYPE?=overwrite

#Image version for Boot usecase
ifeq ($(IMG_TYPE),BOOT)
IMAGE_VERSION=1.0.0

#Image version for update usecase
else ifeq ($(IMG_TYPE),UPDATE)
IMAGE_VERSION=1.1.0

# Start offset of the secondary slot. Required for UPDATE use case with Edge Protect Bootloader.
# Image will be built for start address defined in Linker file. (See APP_START value under IMG_TYPE=BOOT)
# This offset will be used to re-locate the application to secondary slot for update use case. 
# Image will be programmed/downloaded to this address and copied to Primaly slot by Bootloader after verification.
APP_START=0x608C0000

# This options adds the padding to --slot-size bytes
# Also adds trailer magic for the Bootloader to detect a valid updatable image in secondary slot.
ADDITIONAL_ARGS+=--pad
else
$(error Invalid IMG_TYPE. Please set it to either BOOT or UPDATE)
endif

ADDITIONAL_ARGS+=--align $(FLASH_ALIGNMENT) --image-version $(IMAGE_VERSION) \
--min-erase-size $(MIN_ERASE_SIZE) --boot-record $(BOOT_RECORD)

ifeq ($(UPDATE_TYPE),overwrite)
ADDITIONAL_ARGS+=--overwrite-only
else
ADDITIONAL_ARGS+=
endif

#Use protected TLV 0x77 to indicated keyId=1 should be used for Image Verification. 
# KeyId=1 corresponds to OEM_RoT_Key.
# For EPC2 devices, only OEM_RoT_Key usage is supported from Edge Protect Bootloader. 
ADDITIONAL_ARGS+=--protected-tlv 0x77 1

endif

#Add MCUboot metadata/ sign image
POSTBUILD+=cp $(COMPILED_HEX).hex $(COMPILED_HEX)_unsigned.hex 
POSTBUILD+=&&
ifeq ($(SECURED_BOOT),TRUE)
#Signing the image for secured boot
POSTBUILD+=edgeprotecttools sign-image --image $(INPUT_IMAGE) \
                                           --output $(OUTPUT_IMAGE) \
                                           --erased-val $(ERASED_VAL) \
                                           --header-size $(HEADER_SIZE) \
                                           --hex-addr $(APP_START) \
                                           --slot-size $(SLOT_SIZE) \
                                           --key $(KEY_PATH) \
                                           $(ADDITIONAL_ARGS)
                                           
else
#Only MCUboot header is added to the image. No Signature required, if SECURED_BOOT is disabled.
POSTBUILD+=edgeprotecttools image-metadata --image $(INPUT_IMAGE) \
                                           --output $(OUTPUT_IMAGE) \
                                           --erased-val $(ERASED_VAL) \
                                           --header-size $(HEADER_SIZE) \
                                           --slot-size $(SLOT_SIZE) \
                                           --hex-addr $(APP_START) \
                                           $(ADDITIONAL_ARGS)
endif
