TOPDIR = ..
include $(TOPDIR)/Makefile.base


SRC = test.c


TARGET := lib.a
all: $(TARGET)
$(TARGET): $(OBJ)
	$(call summary,AR  ,$@)
	$(Q)$(AR) rc $@ $(OBJ)


