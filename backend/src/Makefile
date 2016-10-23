TARGET := credentarius
ULFIUS_PREFIX := ../3rdparty/ulfius

CFLAGS := -W -Wall
CFLAGS += -I$(ULFIUS_PREFIX)/include

LDFLAGS := -L$(ULFIUS_PREFIX)/lib
LDFLAGS += -lulfius
LDFLAGS += -ljansson
LDFLAGS += -lorcania
LDFLAGS += -lyder
LDFLAGS += -lc
LDFLAGS += -Wl,-rpath=$(ULFIUS_PREFIX)/lib

HEADERS := common.h

OBJS := main.o
OBJS += compile.o
OBJS += mcu.o
OBJS += project.o

all: $(TARGET)

$(TARGET): $(HEADERS) $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	$(RM) $(TARGET) $(OBJS)

