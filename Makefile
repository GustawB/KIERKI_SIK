CC = g++
BOOST_ROOT = /home/gustaw/boost_library/boost_1_74_0
CFLAGS = -Wall -Wextra -O2 -std=c++20 -g -I$(BOOST_ROOT)
LFLAGS = -L/home/gustaw/boost_library/boost_1_74_0/stage/lib -lm -l:libboost_program_options.a -l:libboost_regex.a

.PHONY: all clean

TARGET1 = kierki-serwer
TARGET2 = kierki-klient

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(TARGET1).o common.o regex.o cmd_args_parsers.o senders.o serwer.o points_calculator.o file_reader.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

$(TARGET2): $(TARGET2).o common.o regex.o cmd_args_parsers.o senders.o klient.o klient_printer.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

common.o: common.cpp common.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

regex.o: regex.cpp regex.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

cmd_args_parsers.o: cmd_args_parsers.cpp cmd_args_parsers.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

senders.o: senders.cpp senders.h common.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

points_calculator.o: points_calculator.cpp points_calculator.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

serwer.o: serwer.cpp serwer.h common.h regex.h senders.h points_calculator.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

klient.o: klient.cpp klient.h common.h regex.h senders.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

klient_printer.o: klient_printer.cpp klient_printer.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

file_reader.o: file_reader.cpp file_reader.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

$(TARGET1).o: $(TARGET1).cpp common.h regex.h serwer.h cmd_args_parsers.h senders.h points_calculator.h file_reader.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) $(LFLAGS) -c $< -o $@

$(TARGET2).o: $(TARGET2).cpp common.h regex.h klient.h cmd_args_parsers.h senders.h klient_printer.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) $(LFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET1) $(TARGET2) *.o *~
