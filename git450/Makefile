BIN_PATH := bin
SRC_PATH := src

all: 
	mkdir -p $(BIN_PATH)
	$(MAKE) serverM serverA serverR serverD client

serverM: $(SRC_PATH)/serverM.c
	mkdir -p $(BIN_PATH)
	gcc -g -Wall \
		-o $(BIN_PATH)/serverM \
			$(SRC_PATH)/serverM.c $(SRC_PATH)/utils.c \
		-lpthread

serverA: $(SRC_PATH)/serverA.c
	mkdir -p $(BIN_PATH)
	gcc -g -Wall \
		-o $(BIN_PATH)/serverA \
			$(SRC_PATH)/serverA.c \
			$(SRC_PATH)/utils.c \

serverR: $(SRC_PATH)/serverR.c
	mkdir -p $(BIN_PATH)
	gcc -g -Wall \
		-o $(BIN_PATH)/serverR \
			$(SRC_PATH)/serverR.c \
			$(SRC_PATH)/utils.c \

serverD: $(SRC_PATH)/serverD.c
	mkdir -p $(BIN_PATH)
	gcc -g -Wall \
		-o $(BIN_PATH)/serverD \
			$(SRC_PATH)/serverD.c \
			$(SRC_PATH)/utils.c \

client: $(SRC_PATH)/client.c
	mkdir -p $(BIN_PATH)
	gcc -g -Wall \
		-o $(BIN_PATH)/client \
			$(SRC_PATH)/client.c \
			$(SRC_PATH)/utils.c \
		

clean:
	$(RM) -r $(BIN_PATH) *.dSYM 