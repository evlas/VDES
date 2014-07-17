APP = entrydoor
FILES = configfile gpio entrydoor
CFILES = $(foreach file,$(FILES),$(file).c)
OFILES = $(foreach file,$(FILES),$(file).o)

INCLUDES += -I.
CFLAGS += -fPIC -DPJ_IS_LITTLE_ENDIAN=1 -DPJ_IS_BIG_ENDIAN=0

LIBS += -lstdc++ -lm -lrt -lpthread -lv4l2 -lcrypto -lssl -lbcm2835

all: $(APP)

$(OFILES): %.o: %.c %.h
	gcc $(CFLAGS) $(INCLUDES) $(LIBS) -c -o $@ $< 

$(APP): $(OFILES)
	gcc $(CFLAGS) $(INCLUDES) $(OFILES) $(LIBS) -o $@ `pkg-config --cflags --libs libpjproject`

clean:
	rm -f $(APP) $(OFILES)

