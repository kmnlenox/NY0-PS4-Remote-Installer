TITLE       := NY0 Remote Installer - Diagnostic
VERSION     := 0.1.0
TITLE_ID    := NY0R00001
CONTENT_ID  := IV0000-NY0R00001_00-NY0REMOTEINSTALL

LIBS        := -lc -lkernel -lc++
TOOLCHAIN   := $(OO_PS4_TOOLCHAIN)
PROJDIR     := ny0_remote_installer
INTDIR      := $(PROJDIR)/x64/Debug
CPPFILES    := $(wildcard $(PROJDIR)/*.cpp)
OBJS        := $(patsubst $(PROJDIR)/%.cpp,$(INTDIR)/%.o,$(CPPFILES))
CDIR        := linux
CREATE_EBOOT := $(TOOLCHAIN)/bin/$(CDIR)/create-eboot
PKGTOOL     := $(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core
CREATE_GP4  := tools/create-gp4
PAID        := 0x3800000000000011

CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include
CXXFLAGS    := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

.PHONY: all clean validate
all: $(CONTENT_ID).pkg validate

$(INTDIR):
	mkdir -p $(INTDIR)

$(INTDIR)/%.o: $(PROJDIR)/%.cpp | $(INTDIR)
	clang++ $(CXXFLAGS) -o $@ $<

eboot.bin: $(OBJS)
	ld.lld $(INTDIR)/*.o -o $(INTDIR)/$(PROJDIR).elf $(LDFLAGS)
	$(CREATE_EBOOT) -in=$(INTDIR)/$(PROJDIR).elf -out=$@ -paid=$(PAID)
	test -s $@

sce_sys/param.sfo: Makefile
	mkdir -p sce_sys
	$(PKGTOOL) sfo_new $@
	$(PKGTOOL) sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(PKGTOOL) sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(PKGTOOL) sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(PKGTOOL) sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(PKGTOOL) sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(PKGTOOL) sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(PKGTOOL) sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

$(CONTENT_ID).pkg: eboot.bin sce_sys/param.sfo sce_sys/icon0.png pkg.gp4
	$(PKGTOOL) pkg_build pkg.gp4 .
	test -s $@

pkg.gp4: eboot.bin sce_sys/param.sfo sce_sys/icon0.png tools/create-gp4
	$(CREATE_GP4) -out $@ --content-id=$(CONTENT_ID) --files "eboot.bin sce_sys/param.sfo sce_sys/icon0.png"

validate: $(CONTENT_ID).pkg
	$(PKGTOOL) pkg_validate $(CONTENT_ID).pkg

clean:
	rm -rf $(PROJDIR)/x64 pkg.gp4 sce_sys/param.sfo eboot.bin $(CONTENT_ID).pkg
