#include "hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "buffer.h"


static char *test_table[10] = {
	"asdf", "sadf", "fuck", "shit", "i hate life",
	"fuck myself", "i fucking suck at cs", "asdf;lkj",
	"9", "tenth"
};


void process_test(void *p){
	size_t i = *((size_t*) p);
	free(p);

	pthread_detach(pthread_self());

	//printf("\n\ni: %llu\n\n", i);


	buffer_t *buf = buffer_create(5);

	buffer_append_bytes(buf, test_table[i], strlen(test_table[i]));

    insert(test_table[i], buf);
    //display_table();


	display_table();





	buffer_t *res = get(test_table[i]);





	display_table();






	remove_element(test_table[i]);

	display_table();



	//free_hash_table();
}


//tests the hash_table
int main() {

	for (int i = 0; i < 10; i ++){

		pthread_t id;
		size_t *p = malloc(sizeof(size_t));
		*p = i;
		pthread_create(&id, NULL, process_test, p);
		//process_test(p);
	}
	pthread_exit(NULL);

	//sleep(5);

	free_hash_table();
    return 0;
}
