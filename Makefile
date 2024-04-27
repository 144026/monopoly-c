SUBMOD := src
SRCS := $(foreach mod,$(SUBMOD),$(wildcard $(mod)/*.c))
HEADERS := $(foreach mod,$(SUBMOD),$(wildcard $(mod)/*.h))

OBJS := $(foreach src,$(SRCS),$(patsubst %.c,%.o,$(src)))

PROGS := monopoly

ifeq ($(origin V),command line)
Q =
quiet =
else
Q = @
quiet = quiet
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

all: $(PROGS)

monopoly: $(OBJS)
	$(call cmd,ld)

$(OBJS): %.o: %.c $(HEADERS)
	$(call cmd,cc)

clean:
	$(call cmd,rm,$(OBJS))
	$(call cmd,rm,$(PROGS))

.PHONY: all clean
