TOPDIR = .
include $(TOPDIR)/Makefile.base




# Part 1
# recursive make
.PHONY: subdirs
all clean distclean install uninstall: subdirs

SUBDIRS = widgets
.PHONY: $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(Q)$(MAKE) -C $@ $(MAKECMDGOALS)




SRC = panel.c 
EXTRAOBJ = widgets/lib.a

TARGET := fbpanel
all: $(TARGET)
$(TARGET): $(OBJ)
	$(call summary,LD  ,$@)
	$(Q)$(CC) $(LDFLAGS) $(LIBS) $(OBJ) $(EXTRAOBJ) -o $@
	$(Q)strip $@


