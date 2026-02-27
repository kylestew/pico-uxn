BOARD ?= pico2_w
TEST_TAL ?= hello
BUILD_DIR = build
OPENOCD ?= $(HOME)/.local/bin/openocd

SERIAL_PORT ?= $(shell ls /dev/tty.usbmodem* 2>/dev/null | head -1)

.PHONY: all clean flash debug info serial

all:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DPICO_BOARD=$(BOARD) -DTEST_TAL=$(TEST_TAL) ..
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

info:
	@echo "=== Size ==="
	@arm-none-eabi-size $(BUILD_DIR)/pico_uxn.elf
	@echo ""
	@echo "=== Top 20 symbols ==="
	@arm-none-eabi-nm --size-sort -S -r $(BUILD_DIR)/pico_uxn.elf | head -20

serial:
	minicom -D $(SERIAL_PORT) -b 115200
