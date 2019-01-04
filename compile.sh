# -----
# To run the program after compilation, type
#./server
# and in three different terminal windows:
#./client 0
#./client 1
#./client 2
# -----

gcc -Wall server.c -o server
printf "Server compiled\n"
gcc -Wall client.c -o client -lncurses
printf "Client compiled\n"
