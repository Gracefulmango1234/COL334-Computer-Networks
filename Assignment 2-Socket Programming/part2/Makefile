
all: build

build: client server

client: client.cpp
	$(CXX) -o client client.cpp 

server: server.cpp
	$(CXX) -o server server.cpp 

run: run-server wait run-client wait stop-server

run-server: server
	./server & echo $$! > server_pid.txt  # Run server in the background and save its PID to a file

run-client: client
	./client  # Run the client normally

stop-server:
	@if [ -f server_pid.txt ]; then \
		kill -9 `cat server_pid.txt`; \
		rm -f server_pid.txt; \
		echo "Server stopped."; \
	else \
		echo "Server is not running."; \
	fi

clean:
	rm -f client server server_pid.txt

wait:
	sleep 1

plot:
	python3 plot.py
