TESTS_DIR ?= $(RVV_TESTS_DIR)

include $(TESTS_DIR)/rv32mv/Makefrag

rv32mv_rvv_tests := $(addprefix rv32mv-, $(rv32mv_sc_tests))

rvv_tests := $(rv32mv_rvv_tests)
