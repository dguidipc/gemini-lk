LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/adler32.o\
	$(LOCAL_DIR)/inffast.o\
	$(LOCAL_DIR)/inftrees.o\
	$(LOCAL_DIR)/zutil.o\
	$(LOCAL_DIR)/inflate.o\
	$(LOCAL_DIR)/crc32.o \
	$(LOCAL_DIR)/deflate.o \
	$(LOCAL_DIR)/trees.o
