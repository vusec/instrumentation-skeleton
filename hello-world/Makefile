OBJDIR ?= .

.PHONY: all clean

all: $(OBJDIR)/hello

$(OBJDIR)/%: %.c | $(OBJDIR)
	$(CC) -o $@ $(CFLAGS) $< $(LDFLAGS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -f $(OBJDIR)/hello
