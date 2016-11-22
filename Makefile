
cJSON := lib/cJSON
mongoose := lib/mongoose

all:
	gcc -g -std=c11 -Wall -I $(cJSON) -I $(mongoose) -c $(cJSON)/cJSON.c $(mongoose)/mongoose.c src/*.c
	gcc -lm *.o -o bin/commander
clean:
	rm *.o
	rm bin/commander
