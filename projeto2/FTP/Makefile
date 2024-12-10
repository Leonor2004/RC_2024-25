CC = gcc
CFLAGS = -Wall

SRC = src

URL = ftp://anonymous:anonymous@mirrors.up.pt/debian/README.html

.PHONY: all
all: download

download:  $(SRC)/download.c
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

# netlab1 -> not tested yet, only in RCOM room
# ftp://rcom:rcom@netlab1.fe.up.pt/pipe.txt
# ftp://rcom:rcom@netlab1.fe.up.pt/files/crab.mp4
# ftp://rcom:rcom@netlab1.fe.up.pt/debian/ls-lR.gz
# ftp://rcom:rcom@netlab1.fe.up.pt/pub/parrot/iso/testing/Parrot-architect-5.3_amd64.iso
