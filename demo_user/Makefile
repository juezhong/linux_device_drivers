all: app.c
	arm-linux-gnueabihf-gcc app.c commands.c devices_ioctl.c -o app -g
	cp app ../../drivers/
clean:
	rm -f app *.o *.out
	rm -f compile_commands.json

.PHONY: clean
