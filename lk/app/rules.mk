LOCAL_DIR := $(GET_LOCAL_DIR)

MODULES += \

ifeq ($(DEVICE_TREE_SUPPORT), yes)
MODULES += lib/libfdt
endif

OBJS += \
	$(LOCAL_DIR)/app.o

