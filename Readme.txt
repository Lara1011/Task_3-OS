IPC Task

Part 1: Chat tool that you can use send and recv without the recv will block the code from running.
which means that you can send how many messeges you want and the server will get them all.

Part 2: Preformance toll that checks the speed of diffrent communications.


RUN:

# Create an exe file to run the program
$ make all

# Run the project part A server:
$ ./stnc -s <port>
# Example: ./stnc -s 8080

# Run the project part A client:
$ ./stnc -c <IP> <port>
# Example: ./stnc -c 127.0.0.1 8080

# Run the project part B server:
$ ./stnc -s <port> -p(for preformance check) -q (for quiet mode)
# Example: ./stnc -s 8080 -p -q

# Run the project part B client:
$ ./stnc -c <IP> <port> -p <type> <communication>
# Example: ./stnc -c 127.0.0.1 8080 -p ipv4 tcp

