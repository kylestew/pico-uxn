BOARD ?= pico2_w
BUILD_DIR = build
OPENOCD ?= $(HOME)/.local/bin/openocd

.PHONY: all clean flash debug

all:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DPICO_BOARD=$(BOARD) ..
	@$(MAKE) -C $(BUILD_DIR) -j4

clean:
	@rm -rf $(BUILD_DIR)

flash:
	$(OPENOCD) -f interface/cmsis-dap.cfg -f target/rp2350.cfg \
		-c "adapter speed 5000" \
		-c "program $(BUILD_DIR)/pico_uxn.elf verify reset exit"

debug:
	$(OPENOCD) -f interface/cmsis-dap.cfg -f target/rp2350.cfg \
		-c "adapter speed 5000"
