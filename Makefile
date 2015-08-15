#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
# DATA is a list of directories containing binary files embedded using bin2o
# GRAPHICS is a list of directories containing image files to be converted with grit
#---------------------------------------------------------------------------------
TARGET		:=	bootldr9_rawdevice
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	source
DATA		:=	

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
DEFINES	:=	

ifneq ($(strip $(ENABLE_RETURNFROMCRT0)),)
	DEFINES	:=	$(DEFINES) -DENABLE_RETURNFROMCRT0
endif

ifneq ($(strip $(DEVICEDISABLE_SD)),)
	DEFINES	:=	$(DEFINES) -DDEVICEDISABLE_SD
endif

ifneq ($(strip $(DEVICEDISABLE_NAND)),)
	DEFINES	:=	$(DEFINES) -DDEVICEDISABLE_NAND
endif

ifneq ($(strip $(ENABLE_PADCHECK)),)
	DEFINES	:=	$(DEFINES) -DENABLE_PADCHECK
endif

ifneq ($(strip $(PADCHECK_DISABLEDEVICE_SD)),)
	DEFINES	:=	$(DEFINES) -DPADCHECK_DISABLEDEVICE_SD=$(PADCHECK_DISABLEDEVICE_SD)
endif

ifneq ($(strip $(PADCHECK_ENABLEDEVICE_SD)),)
	DEFINES	:=	$(DEFINES) -DPADCHECK_ENABLEDEVICE_SD=$(PADCHECK_ENABLEDEVICE_SD)
endif

ifneq ($(strip $(PADCHECK_DISABLEDEVICE_NAND)),)
	DEFINES	:=	$(DEFINES) -DPADCHECK_DISABLEDEVICE_NAND=$(PADCHECK_DISABLEDEVICE_NAND)
endif

ifneq ($(strip $(PADCHECK_ENABLEDEVICE_NAND)),)
	DEFINES	:=	$(DEFINES) -DPADCHECK_ENABLEDEVICE_NAND=$(PADCHECK_ENABLEDEVICE_NAND)
endif

ARCH	:=	-marm -fpie

CFLAGS	:=	-g -Wall -Os\
 		-march=armv5te -mtune=arm946e-s -fomit-frame-pointer\
		-ffast-math -std=c99 \
		$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM9 $(DEFINES)
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH) $(DEFINES)
LDFLAGS	=	-nostartfiles -T../bootldr9_rawdevice.ld -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project (order is important)
#---------------------------------------------------------------------------------
LIBS	:= 	-lunprotboot9_sdmmc
 
 
#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(CTRULIB)

ifneq ($(strip $(UNPROTBOOT9_LIBPATH)),)
	LIBDIRS	:=	$(LIBDIRS) $(UNPROTBOOT9_LIBPATH)
endif

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
					$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PNGFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.png)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
 
#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(PNGFILES:.png=.o) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
 
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)
 
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)
 
.PHONY: $(BUILD) clean
 
#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
 
#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).bin

#---------------------------------------------------------------------------------
else
 
#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------

$(OUTPUT).bin	: 	$(OUTPUT).elf
	@$(OBJCOPY) -O binary $< $@
	@echo built ... $(notdir $@)

$(OUTPUT).elf	:	$(OFILES)
 
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)

-include $(DEPSDIR)/*.d
 
#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
