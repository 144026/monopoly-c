SUBMOD := src
SRCS := $(foreach mod,$(SUBMOD),$(wildcard $(mod)/*.c))
HEADERS := $(foreach mod,$(SUBMOD),$(wildcard $(mod)/*.h))

CFLAGS := $(foreach mod,$(SUBMOD),-I$(mod))
CFLAGS += -fcommon
OBJS := $(foreach src,$(SRCS),$(patsubst %.c,%.o,$(src)))

PROGS := monopoly

Q = @
quiet = quiet
ifeq ($(origin V),command line)
ifneq ($(V),)
Q =
quiet =
endif
endif

cmd_run_cc = $(Q)$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<
cmd_cc_quiet = @echo "  CC      $@"

cmd_run_ld = $(Q)$(CC) -o $@ $(LDFLAGS) $^
cmd_ld_quiet = @echo "  LD      $@"

cmd_run_rm = $(Q)$(RM) $(1)
cmd_rm_quiet = @echo "  RM      $(1)"

# https://stackoverflow.com/questions/13260396/gnu-make-3-81-eval-function-not-working
define cmd
$(call cmd_run_$(1),$(2))
$(call cmd_$(1)_$(quiet),$(2))
endef


all: CFLAGS += -g -O2
all: $(PROGS)

# https://stackoverflow.com/questions/64126942/malloc-nano-zone-abandoned-due-to-inability-to-preallocate-reserved-vm-space
# export MallocNanoZone=0
debug: CPPFLAGS += -DGAME_DEBUG
debug: CFLAGS += -fsanitize=address -fno-omit-frame-pointer -g -O1
debug: LDFLAGS += -fsanitize=address
debug: $(PROGS)

monopoly: $(OBJS)
	$(call cmd,ld)

$(OBJS): %.o: %.c $(HEADERS) Makefile
	$(call cmd,cc)

test: all
	@python3 test/autotest.py -d test -n monopoly

clean:
	$(call cmd,rm,$(OBJS))
	$(call cmd,rm,$(PROGS))

.PHONY: all debug test clean
