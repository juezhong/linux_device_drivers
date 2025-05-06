all: app.c
	arm-linux-gnueabihf-gcc app.c commands.c -o app -g
clean:
	rm -f app *.o *.out
	rm -f compile_commands.json

.PHONY: clean
