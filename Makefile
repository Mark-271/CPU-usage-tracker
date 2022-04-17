APP	:= ptrack
CC	:= gcc
LD	:= gcc
CPPFLAGS := -Iinclude -MD
CFLAGS	:= -Wall -Wextra -O2 -g
OBJS	:=		\
	  src/main.o	\
	  src/file.o

# Be silent by default, 'make V=1' shows all compiler calls
ifneq ($(V), 1)
  Q = @
else
  Q =
endif

all: $(APP)

$(APP): $(OBJS)
	@printf "  LD    $@\n"
	$(Q)$(LD) $(LDFLAGS) $(OBJS) -o $(APP)

%.o: %.c
	@printf "  CC    $(*).c\n"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) $< -c -o $@

clean:
	@printf "  CLEAN\n"
	$(Q)-rm -f $(APP)
	$(Q)-rm -f $(OBJS)

.PHONY: all clean

-include $(OBJS:.o=.d)
