CC      = cc
CFLAGS  = -std=c89 -Wall -Wextra -pedantic
LDFLAGS = -pthread

SRC_DIR = src
SEQ_SRC = $(SRC_DIR)/conta-objetos-sequencial.c
PAR_SRC = $(SRC_DIR)/conta-objetos-paralelo.c
SEQ_BIN = conta-objetos-sequencial
PAR_BIN = conta-objetos-paralelo

.PHONY: all clean sequencial paralelo

all: sequencial paralelo

sequencial: $(SEQ_SRC)
	$(CC) $(CFLAGS) $< -o $(SEQ_BIN)

paralelo: $(PAR_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $(PAR_BIN)

clean:
	rm -f $(SEQ_BIN) $(PAR_BIN)