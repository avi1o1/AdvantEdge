# Compiler and flags
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -pthread -lresolv

# Directories
SRC_DIR = nfs
COMMON_DIR = $(SRC_DIR)/common
NM_DIR = $(SRC_DIR)/naming_server
SS_DIR = $(SRC_DIR)/storage_server
CLIENT_DIR = $(SRC_DIR)/client
MOST_WANTED_DIR = most_wanted

# Include directories
COMMON_INC = $(COMMON_DIR)/include
NM_INC = $(NM_DIR)
SS_INC = $(SS_DIR)
CLIENT_INC = $(CLIENT_DIR)
MOST_WANTED_INC = $(MOST_WANTED_DIR)

# Update CFLAGS with all include paths
CFLAGS += -I$(COMMON_INC) -I$(NM_INC) -I$(SS_INC) -I$(CLIENT_INC) -I$(MOST_WANTED_INC)

# Source files
COMMON_SRC = $(wildcard $(COMMON_DIR)/src/*.c)
NM_SRC = $(wildcard $(NM_DIR)/*.c)
SS_SRC = $(wildcard $(SS_DIR)/*.c)
CLIENT_SRC = $(wildcard $(CLIENT_DIR)/*.c)
MOST_WANTED_SRC = $(wildcard $(MOST_WANTED_DIR)/*.c)

# Object files
COMMON_OBJ = $(COMMON_SRC:.c=.o)
NM_OBJ = $(NM_SRC:.c=.o)
SS_OBJ = $(SS_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
MOST_WANTED_OBJ = $(MOST_WANTED_SRC:.c=.o)

# Executables
NM_EXEC = naming_server.out
SS_EXEC = storage_server.out
CLIENT_EXEC = client.out

# Targets
all: $(NM_EXEC) $(SS_EXEC) $(CLIENT_EXEC)

# Naming Server
$(NM_EXEC): $(COMMON_OBJ) $(MOST_WANTED_OBJ) $(NM_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# Storage Server
$(SS_EXEC): $(COMMON_OBJ) $(MOST_WANTED_OBJ) $(SS_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# Client
$(CLIENT_EXEC): $(COMMON_OBJ) $(MOST_WANTED_OBJ) $(CLIENT_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# Object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(COMMON_OBJ) $(NM_OBJ) $(SS_OBJ) $(CLIENT_OBJ) $(MOST_WANTED_OBJ) $(NM_EXEC) $(SS_EXEC) $(CLIENT_EXEC) .metadata nfs.log
	rm -rf most_wanted
	mkdir most_wanted
	cp Kalimba.mp3 most_wanted/

# Docs
clean_docs:
	rm -rf docs

docs: clean_docs
	doxygen Doxyfile

.PHONY: all clean