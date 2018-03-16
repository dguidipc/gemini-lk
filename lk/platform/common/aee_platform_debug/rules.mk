LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/../include
INCLUDES += -I$(LOCAL_DIR)/../../$(PLATFORM)/include

OBJS += \
	$(LOCAL_DIR)/systracker.o\
	$(LOCAL_DIR)/latch.o\
	$(LOCAL_DIR)/dfd.o\
	$(LOCAL_DIR)/etb.o
