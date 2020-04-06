default: all

###############################################################################
# Variables

# Override this target to build for native
Target := stm32f303
#Target = native

Buildtype := debug # release

###############################################################################
# Sources

Controller_Sources = \
	main.c \
	controller.c \
	ihm_communication.c \


###############################################################################
# Generic definitions
OBJDIR=obj/$(Target)
# CFlags are common to C and C++
CFlags =
CXXFlags =
LFlags =

# Warning levels
CFlags += \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	#-werror

# Generic flags
CFlags += \
	-std=gnu11 \
	-fdiagnostics-color=always \
	-Os \
	-g

CXXFlags += \
	-std=gnu++14


# Dependencies on headers
CFlags += -MMD -MP



###############################################################################
# Target-specific definitions

CFlags += -Ilowlevel/include

LowLevel_Sources =
LowLevel_Objects =
-include lowlevel/$(Target)/$(Target).mk

###############################################################################
# Define object files and executables after everything

LowLevel_Objects+= $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(LowLevel_Sources)))
LowLevel_Depends = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.d,$(LowLevel_Sources)))
-include $(LowLevel_Depends)

Controller_Objects = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(Controller_Sources)))
Controller_Depends = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.d,$(Controller_Sources)))
-include $(Controller_Depends)

.PRECIOUS: $(OBJDIR)/. $(OBJDIR)%/.

$(OBJDIR)/.:
	echo $@
	mkdir -p $@

$(OBJDIR)%/.:
	echo $@
	mkdir -p $@

.SECONDEXPANSION:


# Generic rules


$(OBJDIR)/%.o: %.cpp Makefile | $$(@D)/.
	@echo CXX $<
	@$(CXX) $(CFlags) $(CXXFlags) $< -o $@ -c

$(OBJDIR)/%.o: %.c Makefile | $$(@D)/.
	@echo CC $<
	@$(CC) $(CFlags) $< -o $@ -c

$(OBJDIR)/%.o: %.s Makefile | $$(@D)/.
	@echo CC $<
	@$(CC) $(CFlags) $< -o $@ -c -x assembler-with-cpp

%.elf:
	@echo LINK $@
	@$(CC) $(CFlags) $^ $(LFlags) -o $@

 
Controller.$(Target).elf: $(Controller_Objects) $(LowLevel_Objects)


all: Controller.$(Target).elf


clean:
		find . \
		\( -not -path  "./hal_common*" \) \
		\( -name "*.o" \
		-o -name "*.a" \
		-o -name "*.d" \
		-o -name "*.hex" \
		-o -name "*.elf" \
		\) -delete
