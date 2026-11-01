# NY0 identity-matched minimal launch diagnostic.
# Run from the ps4/ directory so package paths match the production build.
TITLE       := NY0 Remote Installer
VERSION     := 01.00
TITLE_ID    := NYOR00001
CONTENT_ID  := IV0000-NYOR00001_00-NY0REMOTEINSTALL

TOOLCHAIN   := $(OO_PS4_TOOLCHAIN)
PROJECT     := diagnostics/ny0_identity_test
INTDIR      := $(PROJECT)/x64/Debug
OBJECT      := $(INTDIR)/main.o
ELF         := $(INTDIR)/identity_test.elf
OELF        := $(INTDIR)/identity_test.oelf
LIBS        := -lc -lkernel -lc++
CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include
CXXFLAGS    := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o
PKGTOOL     := $(TOOLCHAIN)/bin/linux/PkgTool.Core
CREATE_FSELF := tools/create-fself
CREATE_GP4  := tools/create-gp4

.PHONY: all clean
all: $(CONTENT_ID).pkg

$(INTDIR):
	mkdir -p $(INTDIR)

$(OBJECT): $(PROJECT)/main.cpp | $(INTDIR)
	clang++ $(CXXFLAGS) -o $@ $<

eboot.bin: $(OBJECT)
	ld.lld $(OBJECT) -o $(ELF) $(LDFLAGS)
	$(CREATE_FSELF) -in=$(ELF) -out=$(OELF) --eboot "eboot.bin" --paid 0x3800000000000011
	test -s $@

sce_sys/param.sfo: diagnostics/ny0_identity_test/Makefile
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

$(CONTENT_ID).pkg: eboot.bin sce_sys/param.sfo sce_sys/icon0.png sce_sys/about/right.sprx sce_module/libc.prx sce_module/libSceFios2.prx web/index.html web/style.css web/app.js pkg.gp4
	$(PKGTOOL) pkg_build pkg.gp4 .
	$(PKGTOOL) pkg_validate $@

pkg.gp4: eboot.bin sce_sys/param.sfo sce_sys/icon0.png sce_sys/about/right.sprx sce_module/libc.prx sce_module/libSceFios2.prx web/index.html web/style.css web/app.js
	$(CREATE_GP4) -out $@ --content-id=$(CONTENT_ID) --files "eboot.bin sce_sys/param.sfo sce_sys/icon0.png sce_sys/about/right.sprx sce_module/libc.prx sce_module/libSceFios2.prx web/index.html web/style.css web/app.js"

clean:
	rm -rf $(INTDIR) pkg.gp4 sce_sys/param.sfo eboot.bin $(CONTENT_ID).pkg
