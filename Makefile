#
# nmap, iperf3 ubuntu 패키지가 필요함.
# iperf3 package는 서버와 동일한 package로 설치하여야 속도가 정상적으로 나옴.
#
CC      = gcc
CFLAGS  = -W -Wall -g
CFLAGS  += -D__DEBUG_APP__

INCLUDE = -I/usr/local/include
LDFLAGS = -L/usr/local/lib -lpthread

# 폴더이름으로 실행파일 생성
TARGET  := $(notdir $(shell pwd))

# 정의되어진 이름으로 실행파일 생성
# TARGET := test

SRC_DIRS = .
# SRCS     = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
SRCS     = $(shell find . -name "*.c")
OBJS     = $(SRCS:.c=.o)

all : $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f $(OBJS)
	rm -f $(TARGET)
