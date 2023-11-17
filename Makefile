include common.mak

ifeq ($(TARGET),linux)
$(info Running Linux build)
include linux.mak
else ifeq ($(TARGET),win)
$(info running Windows build)
include win.mak
else ifeq ($(TARGET),mac)
$(info Running Mac build)
else
TARGET=linux
include linux.mak
$(info Assuming target is $(TARGET))
endif

all: $(TARGET)
