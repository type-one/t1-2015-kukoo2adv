#
#
# NOTE: for educational purposes only. For real work, use
# devkitPro's templates ( $(DEVKITPRO)/examples/gba/template )
# or tonclib's templates ( $(TONCCODE)/lab/template )

# decreasing executable size with gcc options
# http://devkitpro.org/viewtopic.php?f=14&t=1665

#PATH := $(DEVKITARM)/bin:$(DEVKITPRO)/tools/bin:$(PATH)
PATH := $(DEVKITPRO)/devkitARM_r42/bin:$(DEVKITPRO)/tools/bin:$(PATH)
#PATH := $(DEVKITPRO)/devkitARM_r45/bin:$(DEVKITPRO)/tools/bin:$(PATH)

# --- Project details -------------------------------------------------

PROJ    := kukoo2adv
TITLE   := $(PROJ)

COBJS   := kukoo2adv.o
ICOBJS  :=
AOBJS   := crt0.o tonc_bios.o tonc_memcpy.o music.o samples.o instruments.o
ALIBS   := libkrawall.a


OBJS	:= $(AOBJS) $(COBJS) $(ICOBJS)


LINKSCRIPT   := lnkscript

# --- boot type (MB=0 : normal. MB=1 : multiboot) ---
MB = 0

ifeq ($(MB),1)

TARGET	:= $(PROJ).mb
SPECS	:= -T$(LINKSCRIPT) -nostartfiles

else

TARGET	:= $(PROJ)
SPECS	:= -T$(LINKSCRIPT) -nostartfiles

endif

# --- Compiling -------------------------------------------------------

CROSS	?= arm-none-eabi-
AS		:= $(CROSS)as
CC		:= $(CROSS)gcc
LD		:= $(CROSS)gcc
OBJCOPY	:= $(CROSS)objcopy


ARCH	:= -mthumb-interwork -mthumb
RARCH := -mthumb-interwork -mthumb
IARCH := -mthumb-interwork -marm

#https://gcc.gnu.org/onlinedocs/gcc-4.7.1/gcc/Optimize-Options.html

#final compo optimisation
CBASE = -O3 -mcpu=arm7tdmi -mtune=arm7tdmi -fno-strict-aliasing -ffast-math -funsafe-math-optimizations \
        -fno-exceptions -funsafe-loop-optimizations -foptimize-register-move -fomit-frame-pointer \
        -fno-trapping-math -funroll-loops -foptimize-register-move -std=gnu99
 

ASFLAGS	:= -mthumb-interwork
RCFLAGS := $(CBASE) $(RARCH) -ffixed-r14
ICFLAGS := $(CBASE) $(IARCH) -mlong-calls
CFLAGS	:= $(RCFLAGS)

#regular size optimization
LDFLAGS	:= $(ARCH) $(SPECS) -s -lm

.PHONY : build clean

# --- Build -----------------------------------------------------------

build : $(TARGET).gba

$(TARGET).gba : $(TARGET).elf
	$(CROSS)strip -s -g -S -d --strip-debug --strip-unneeded $<
	$(OBJCOPY) -v -O binary $< $@
	-@gbafix $@ -t$(TITLE)

$(TARGET).elf : $(OBJS)
	$(LD) $^ $(ALIBS) $(LDFLAGS) -o $@

$(ICOBJS) : %.iwram.o : %.iwram.c
	$(CC) $(ICFLAGS) -c $< -o $@

$(COBJS) : %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(AOBJS) : %.o : %.s
	$(AS) $(ASFLAGS) $< -o $@

# --- Clean -----------------------------------------------------------

clean :
	@rm -fv *.gba
	@rm -fv *.elf
	@rm -fv *.o

#EOF
