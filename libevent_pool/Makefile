
all:
	g++ -g -o server server.cc -lpthread -levent -O2
	g++ -g -o client client.cc -O2
	g++ -g -o test_long_conn test_long_conn.cc -O2
	g++ -g -o test_short_conn test_short_conn.cc -O2
	g++ -g -o test_conn_num test_conn_num.cc -O2

clean:
	rm -f server client test_long_conn test_conn_num