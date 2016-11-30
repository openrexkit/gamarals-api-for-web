FLASHER=echo

all: .firmware.bin

.firmware.bin: $(wildcard *.c *.h)
	@echo "**********************"
	@echo "*  Compile  Started  *"
	@echo "**********************"
	@echo

	$(CC) -o .firmware.bin $^

	@echo
	@echo "**********************"
	@echo "*      Success!      *"
	@echo "**********************"

clean:
	@$(RM) *.o .firmware.bin

flash: .firmware.bin
	@echo "********************"
	@echo "*  Flash  Started  *"
	@echo "********************"
	@echo

	@$(FLASHER) flash .firmware.bin

	@echo
	@echo "********************"
	@echo "*     Success!     *"
	@echo "********************"

reboot:
	@echo "********************"
	@echo "*  Reboot Started  *"
	@echo "********************"
	@echo

	@$(FLASHER) reboot

	@echo
	@echo "********************"
	@echo "*     Success!     *"
	@echo "********************"
