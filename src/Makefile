DEBUG = y
ifeq ($(DEBUG),y)
DEBFLAGS = -g -O0 -DDEBUG # "-O" is needed to expand inlines
endif

COPTS += -Wall #-S
CFLAGS += $(COPTS) $(DEBFLAGS)

LINK_OPTS = -pthread

BINS := bank
bank_OBJS := bank.o accdb.o wal.o tcp_server.o handler.o user.o idle.o replay.o
OBJS := $(bank_OBJS)

.PHONY: all clean

all: $(BINS)

bank: $(bank_OBJS)
	$(CC) $(LINK_OPTS) $^ -o $@

clean:
	rm -f $(BINS) *.o *.d

# rules

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	$(CC) -M $(CFLAGS) $*.c > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

-include $(OBJS:.o=.d)
