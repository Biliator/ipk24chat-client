CFLAGS=-std=gnu17 -Wall -D_GNU_SOURCE -Wextra -Werror
CFLAGS_EZ=-std=gnu17 -Werror -D_GNU_SOURCE
FILES=ipk24chat-client.c udp.c udp_fifo.c udp_id_history.c tcp.c
NAME=ipk24chat-client

compile:
	gcc $(CFLAGS) $(FILES) -o $(NAME)