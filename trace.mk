
CFLAGS += -finstrument-functions -g -O0
LDFLAGS += -rdynamic -funwind-tables
LDFLAGS += -Wl,--wrap=fork -Wl,--wrap=strcmp -Wl,--wrap=strncmp
LDFLAGS += -Wl,--wrap=strcpy -Wl,--wrap=strncpy
LDFLAGS += -Wl,--wrap=open -Wl,--wrap=close
LDFLAGS += -Wl,--wrap=fopen -Wl,--wrap=fclose
LDFLAGS += -Wl,--wrap=popen -Wl,--wrap=pclose
LDFLAGS += -Wl,--wrap=read -Wl,--wrap=write
LDFLAGS += -Wl,--wrap=fread -Wl,--wrap=fwrite
LDFLAGS += -Wl,--wrap=system 
LDFLAGS += -Wl,--wrap=sleep
LDFLAGS += -Wl,--wrap=pthread_create

