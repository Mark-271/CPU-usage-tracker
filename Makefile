APP	:= ptrack
CSTD	?= -std=gnu99
CPPFLAGS := -Iinclude -MD
CFLAGS	:= -Wall -Wextra -O2 -pthread $(CSTD)
LDLIBS	:= -pthread
OBJS	:=			\
	  src/main.o		\
	  src/tools.o		\
	  src/tracker.o

BUILD	?= debug
ifeq ($(BUILD), release)
  CPPFLAGS += -DNDEBUG
  CFLAGS += -s
else ifeq ($(BUILD), debug)
  CFLAGS += -g
else
  $(error Incorrect BUILD variable)
endif

# Be silent by default, 'make V=1' shows all compiler calls
ifneq ($(V), 1)
  Q = @
else
  Q =
endif

all: $(APP)

$(APP): $(OBJS)
	@printf "  LD    $@\n"
	$(Q)$(CC) $(LDFLAGS) $(OBJS) -o $(APP) $(LDLIBS)

%.o: %.c
	@printf "  CC    $(*).c\n"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) $< -c -o $@

clean:
	@printf "  CLEAN\n"
	$(Q)-rm -f $(APP)
	$(Q)-rm -f $(OBJS)
	$(Q)find src/ -name '*.d' -exec rm -f {} \;

.PHONY: all clean

-include $(OBJS:.o=.d)
