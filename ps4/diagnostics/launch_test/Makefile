# Minimal OpenOrbis launch smoke test.
TITLE       := NY0 Launch Test
VERSION     := 1.00
TITLE_ID    := NYOD00001
CONTENT_ID  := IV0000-NYOD00001_00-NY0LAUNCHTEST000

TOOLCHAIN   := $(OO_PS4_TOOLCHAIN)
INTDIR      := x64/Debug
LIBS        := -lc -lkernel -lc++
CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include
CXXFLAGS    := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o
PKGTOOL     := $(TOOLCHAIN)/bin/linux/PkgTool.Core

.PHONY: all clean
all: $(CONTENT_ID).pkg

$(INTDIR):
	mkdir -p $(INTDIR)

$(INTDIR)/main.o: main.cpp | $(INTDIR)
	clang++ $(CXXFLAGS) -o $@ $<

eboot.bin: $(INTDIR)/main.o
	ld.lld $(INTDIR)/main.o -o $(INTDIR)/launch_test.elf $(LDFLAGS)
	tools/create-fself -in=$(INTDIR)/launch_test.elf -out=$(INTDIR)/launch_test.oelf --eboot "eboot.bin" --paid 0x3800000000000011
	test -s $@

sce_sys/param.sfo: Makefile
	mkdir -p sce_sys
	$(PKGTOOL) sfo_new $@
	$(PKGTOOL) sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(PKGTOOL) sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(PKGTOOL) sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(PKGTOOL) sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(PKGTOOL) sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(PKGTOOL) sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(PKGTOOL) sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

$(CONTENT_ID).pkg: eboot.bin sce_sys/param.sfo sce_sys/icon0.png pkg.gp4
	$(PKGTOOL) pkg_build pkg.gp4 .
	$(PKGTOOL) pkg_validate $@

pkg.gp4: eboot.bin sce_sys/param.sfo sce_sys/icon0.png
	tools/create-gp4 -out $@ --content-id=$(CONTENT_ID) --files "eboot.bin sce_sys/param.sfo sce_sys/icon0.png"

clean:
	rm -rf x64 pkg.gp4 sce_sys/param.sfo eboot.bin $(CONTENT_ID).pkg
