COMMON_FILES=../proto/proto.c
CLIENT_FILES=resources.c login.c chat.c messages.c client_main.c $(COMMON_FILES)
SERVER_FILES=messages.c chat.c server_main.c $(COMMON_FILES)
GCC_FLAGS= -std=c99 -Wall -Wextra -pedantic -O2 -pthread -I../proto

all: chatserver chatclient

chatserver:
	cd server; gcc $(GCC_FLAGS)  $(SERVER_FILES) -lsqlite3 -o ../chat_server
chatclient:
	cd client; glib-compile-resources client.gresource.xml --target=resources.c --generate-source
	cd client; gcc $(GCC_FLAGS) $(CLIENT_FILES) `pkg-config --cflags --libs gtk+-3.0` -o ../chat_client
clean:
	rm chat_client chat_server client/resources.c
