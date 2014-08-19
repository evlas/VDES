all: video_entryphone

video_entryphone: video_entryphone.c
	cc -o $@ $< `pkg-config --cflags --libs libpjproject`

clean:
	rm -rf video_entryphone
