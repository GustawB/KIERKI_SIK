CC = g++
BOOST_ROOT = /home/gustaw/boost_library/boost_1_74_0
CFLAGS = -Wall -Wextra -O2 -std=c++20 -I$(BOOST_ROOT)
LFLAGS = -L/home/gustaw/boost_library/boost_1_74_0/stage/lib -lm -l:libboost_program_options.a -l:libboost_regex.a

.PHONY: all clean

TARGET1 = kierki-serwer
TARGET2 = kierki-klient

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(TARGET1).o common.o regex.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

$(TARGET2): $(TARGET2).o common.o regex.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

common.o: common.cpp common.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

regex.o: regex.cpp regex.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) -c $< -o $@

$(TARGET1).o: $(TARGET1).cpp common.h regex.h serwer.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) $(LFLAGS) -c $< -o $@

$(TARGET2).o: $(TARGET2).cpp common.h regex.h klient.h
	$(CC) $(CFLAGS) -I$(BOOST_ROOT) $(LFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET1) $(TARGET2) *.o *~
