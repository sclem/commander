
cJSON := lib/cJSON
mongoose := lib/mongoose

all:
	gcc -g -Wall -I $(cJSON) -I $(mongoose) -c $(cJSON)/cJSON.c $(mongoose)/mongoose.c src/*.c
	gcc -lm *.o -o bin/commander
clean:
	rm *.o
	rm bin/commander
