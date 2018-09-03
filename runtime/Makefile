# overwritten in setup.py
OBJDIR         ?= obj
LLVM_VERSION   ?= 4.0.0

PKG_CONFIG     := python3 ../setup.py pkg-config
BUILTIN_CFLAGS := `$(PKG_CONFIG) llvm-passes-builtin-$(LLVM_VERSION) --runtime-cflags`

CXX      := g++
CXXFLAGS := -std=c++14 -O2 -fpic -Wall -Wextra -march=native $(BUILTIN_CFLAGS)
LIB      := libcount.so
OBJS     := count.o

.PHONY: all clean

all: $(OBJDIR)/$(LIB)

$(OBJDIR)/$(LIB): $(addprefix $(OBJDIR)/,$(OBJS))
	$(CXX) -shared -static-libstdc++ -o $@ $^

$(OBJDIR)/%.o: %.cxx | $(OBJDIR)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(OBJDIR):
	mkdir -p $@

clean:
	rm -rf $(OBJDIR)
