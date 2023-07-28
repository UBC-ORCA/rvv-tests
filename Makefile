TESTS_DIR := .
COMMON_DIR := $(TESTS_DIR)/common

include $(COMMON_DIR)/rvv_tests.mk

rv32mv_rvv_tests := $(addprefix rv32mv-, $(rv32mv_sc_tests))

default: all

#--------------------------------------------------------------------
# Build rules
#--------------------------------------------------------------------

DEFINES       ?=

RISCV_XLEN    ?= 32
RISCV_PREFIX  ?= riscv$(RISCV_XLEN)-unknown-elf-

RISCV_CC      ?= $(RISCV_PREFIX)gcc
RISCV_OBJDUMP ?= $(RISCV_PREFIX)objdump
RISCV_SIM     ?= spike

RISCV_WARNINGS += -Wunused-variable -Wall -Wextra -Wno-unused-command-line-argument # -Werror

RISCV_CCFLAGS ?= --specs=picolibc.specs --crt0=hosted -mcmodel=medlow -I$(COMMON_DIR) -static -O2 -fno-tree-vectorize -ffunction-sections $(DEFINES) $(RISCV_WARNINGS)
RISCV_LDFLAGS ?= -Xlinker --defsym=__mem_size=131072 -Wl,--print-memory-usage
RISCV_OBJDUMPFLAGS ?= --disassemble-all --disassemble-zeroes --section=.text --section=.text.startup --section=.text.init --section=.data

vpath %.c $(TESTS_DIR)

#------------------------------------------------------------
# Build assembly tests

%.dump: %
	$(RISCV_OBJDUMP) $(RISCV_OBJDUMPFLAGS) $< > $@

%.out: %
	$(RISCV_SIM) --isa=rv64gc_zfh_zicboz_svnapot_zicntr --misaligned $< 2> $@

%.out32: %
	$(RISCV_SIM) --isa=rv32gc_zfh_zicboz_svnapot_zicntr --misaligned $< 2> $@

define compile_template

$$($(1)_rvv_tests): $(1)-%: $(1)/%.c
	$$(RISCV_CC) $(2) -I$$(CURDIR)/macros/vector $$(RISCV_CCFLAGS) $$(RISCV_LDFLAGS) -o $$@ $$< $(COMMON_DIR)/boardsupport.c 
$(1)_tests += $$($(1)_rvv_tests)

$(1)_tests_dump = $$(addsuffix .dump, $$($(1)_tests))

$(1): $$($(1)_tests_dump)

.PHONY: $(1)

COMPILER_SUPPORTS_$(1) := $$(shell $$(RISCV_CC) $(2) -c -x c /dev/null -o /dev/null 2> /dev/null; echo $$$$?)

ifeq ($$(COMPILER_SUPPORTS_$(1)),0)
tests += $$($(1)_tests)
endif

endef

$(eval $(call compile_template,rv32mv,-march=rv32imav -mabi=ilp32))

tests_dump = $(addsuffix .dump, $(tests))
tests_hex = $(addsuffix .hex, $(tests))
tests_out = $(addsuffix .out, $(filter rv64%,$(tests)))
tests32_out = $(addsuffix .out32, $(filter rv32%,$(tests)))

run: $(tests_out) $(tests32_out)

junk += $(tests) $(tests_dump) $(tests_hex) $(tests_out) $(tests32_out)

#------------------------------------------------------------
# Default

all: $(tests_dump) move_buildfiles

builddir=build

.PHONY: move_buildfiles
move_buildfiles:
	@mkdir -p $(builddir)
	@mkdir -p $(builddir)/bin
	@for file in $(junk); do \
		if [ -e "$$file" ]; then \
			if [ -x "$$file" ]; then \
				mv "$$file" $(builddir)/bin/$$file; \
			else \
				mv "$$file" $(builddir)/$$file; \
        		fi; \
        	fi; \
    	done

#------------------------------------------------------------
# Clean up

clean:
	rm -rf $(builddir)
