CROSS_COMPILE=arm-linux-gnueabihf-
CC=$(CROSS_COMPILE)gcc
STRIP=$(CROSS_COMPILE)strip
APPLICATION=levelupx-1
OUTPUT_DIR=./image
MOUNT_POINT=./mount
IMAGE=./module-levelupx-1.img

.PHONY: clean-application clean-image clean image

all: image

application:
	$(CC) $(CFLAGS) src/levelupx-1.c -o $(OUTPUT_DIR)/$(APPLICATION)
	$(STRIP) $(OUTPUT_DIR)/$(APPLICATION)

image: application
	dd if=/dev/zero of=$(IMAGE) bs=1M count=16
	mkfs.ext4 $(IMAGE)
	sudo mount -o loop $(IMAGE) $(MOUNT_POINT)
	sudo cp -rv $(OUTPUT_DIR)/* $(MOUNT_POINT)
	sudo chmod +s $(MOUNT_POINT)/$(APPLICATION)
	sudo umount $(MOUNT_POINT)

clean-application:
	rm -f $(OUTPUT_DIR)/$(APPLICATION)

clean-image:
	rm -f $(IMAGE)

clean: clean-application clean-image