CC = gcc
CFLAGS = -Wall


URL = ftp://anonymous:anonymous@mirrors.up.pt/debian/README.html

.PHONY: all
all: download

download:  download.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: run
run: download
	./$< $(URL)

.PHONY: clean
clean:
	rm -f download

# Files to download

# ftp://ftp.up.pt/pub/gnu/emacs/elisp-manual-21-2.8.tar.gz -> working
# ftp://demo:password@test.rebex.net/readme.txt -> working
# ftp://anonymous:anonymous@ftp.bit.nl/speedtest/100mb.bin -> working

# ftp://anonymous:anonymous@mirrors.up.pt/debian/README.html -> working

# netlab1 -> deuuuuuuuu
# ftp://rcom:rcom@ftp.netlab.fe.up.pt/README
