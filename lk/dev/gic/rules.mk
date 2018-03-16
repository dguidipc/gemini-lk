LOCAL_DIR := $(GET_LOCAL_DIR)

ifeq ($(MTK_GIC_VERSION), v3)
	OBJS += $(LOCAL_DIR)/mt_gic_v3.o
else ifeq ($(MTK_GIC_VERSION), v2)
	OBJS += $(LOCAL_DIR)/mt_gic_v2.o
endif

INCLUDES += -I$(LOCAL_DIR)/include
