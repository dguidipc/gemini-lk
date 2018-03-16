LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/../include
INCLUDES += -I$(LOCAL_DIR)/../../$(PLATFORM)/include
INCLUDES += -I$(LOCAL_DIR)/../../$(PLATFORM)/include/platform

OBJS += \
	$(LOCAL_DIR)/ufs_aio_core.o \
	$(LOCAL_DIR)/ufs_aio_error.o \
	$(LOCAL_DIR)/ufs_aio_hcd.o \
	$(LOCAL_DIR)/ufs_aio_interface.o \
	$(LOCAL_DIR)/ufs_aio_quirks.o \
	$(LOCAL_DIR)/ufs_aio_rpmb.o \
	$(LOCAL_DIR)/ufs_aio_utils.o
