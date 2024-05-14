CC = g++
BOOST_ROOT = /home/gustaw/boost_library/boost_1_74_0
CFLAGS = -Wall -Wextra -O2 -std=c++20 -I$(BOOST_ROOT)
LFLAGS = -L/home/gustaw/boost_library/boost_1_74_0/stage/lib -lm -l:libboost_program_options.a -l:libboost_regex.a

.PHONY: all clean

TARGET1 = kierki-serwer
TARGET2 = kierki-klient

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(TARGET1).o common.o regex.o cmd_args_parsers.o senders.o retrievers.o serwer.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

$(TARGET2): $(TARGET2).o common.o regex.o cmd_args_parsers.o senders.o retrievers.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

common.o: common.cpp common.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

regex.o: regex.cpp regex.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

cmd_args_parsers.o: cmd_args_parsers.cpp cmd_args_parsers.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

senders.o: senders.cpp senders.h common.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

retrievers.o: retrievers.cpp retrievers.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

serwer.o: serwer.cpp serwer.h common.h regex.h senders.h retrievers.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

$(TARGET1).o: $(TARGET1).cpp common.h regex.h serwer.h cmd_args_parsers.h senders.h retrievers.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) $(LFLAGS) -c $< -o $@

$(TARGET2).o: $(TARGET2).cpp common.h regex.h klient.h cmd_args_parsers.h senders.h retrievers.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) $(LFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET1) $(TARGET2) *.o *~
