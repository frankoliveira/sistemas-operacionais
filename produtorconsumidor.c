 #include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#define true 1
#define false 0

pthread_mutex_t mutex; //acesso exclusivo ao buffer
sem_t semaphore_full; //espacos preechidos
sem_t semaphore_empty; //espacos vazios
int produce_num_itens; //quantidade produzida pelo produtor
int consume_num_itens; //quantidade consumida pelo consumidor

typedef struct {
    int *buffer;
    int counter;
    int size;
} Buffer;
Buffer buffer;

void *producerFunc(void *args);
void *consumerFun(void *args);
void pushItem(Buffer *buff, int item);
int popItem(Buffer *buff);
int *creatBuffer(int size);
void printBuffer(int *vet, int size);

int main(int argc, char const *argv[]){

	srand(time(NULL));

	if(argc != 4){
		printf("ERROR: incorrect number of arguments\n");
		exit(0);
		//argv deve conter 4 argumentos: nome do programa e três argumentos(tamanho do buffer, quantidade produtor, quantidade consumidor)
	}

	buffer.size = atoi(argv[1]); //tamanho do buffer
	produce_num_itens = atoi(argv[2]); //itens produzidos por vez pelo produtor
	consume_num_itens = atoi(argv[3]); //itens consumidos por vez pelo consumidor

	if(buffer.size <= 0){
		printf("ERROR: Buffer size can't be zero or negative\n");
		exit(0);
	}
	else if(produce_num_itens<=0 || consume_num_itens<=0){
		printf("ERROR: Producer/Consumer can't be zero or have negative values\n");
		exit(0);
	}
	else if(produce_num_itens>buffer.size || consume_num_itens>buffer.size){
		printf("ERROR: Input for producer/consumer exceeds the buffer size\n");
		exit(0);
	}

	buffer.buffer = creatBuffer(buffer.size); //inicia o buffer com zeros
	buffer.counter = 0;

	printf("-----------start-----------\n");

	pthread_mutex_init(&mutex, NULL); //inicia o mutex


	sem_init(&semaphore_empty , 0, buffer.size); //inicia o semaforo correspondente ao espaço disponivel
	sem_init(&semaphore_full , 0, 0); //inicia o semaforo correspondente ao recurso disponível para consumo

	pthread_t threadProducer;
	pthread_t threadConsumer; 

	if(pthread_create(&threadProducer, NULL, &producerFunc, NULL) != 0){

		perror("ERROR: Failed to create Producer thread");
		exit(0);
	} //inicia a thread do produtor


	if(pthread_create(&threadConsumer, NULL, &consumerFun, NULL) != 0){
		perror("ERROR: Failed to create Consumer thread");
		exit(0);
	} //inicia a thread do consumidor


	if(pthread_join(threadProducer, NULL) != 0){
		perror("ERROR: Failed to join Producer hread");
		exit(0);
	}

	if(pthread_join(threadConsumer, NULL) != 0){
		perror("ERROR: Failed to join Consumer thread");
		exit(0);
	}

	pthread_mutex_destroy(&mutex);
	sem_destroy(&semaphore_full);
	sem_destroy(&semaphore_empty);
	free(buffer.buffer);
	
	return 0;
}

void *producerFunc(void *args){

	int i, item;
	int inCriticRegion = false;

	while(true){

		for( i=0; i < produce_num_itens; i++ ){

			sem_wait(&semaphore_empty); // se não tem espaço suficiente, a thread fica bloqueada esperando o recurso

			if(!inCriticRegion){
				pthread_mutex_lock(&mutex); // bloqueia a região crítica para inserir no buffer
				inCriticRegion = true;
			}

			item = ((rand()%buffer.size)+1);
			pushItem(&buffer, item); // insere itens
			printf("Produced item [%d] - ", item);
			printBuffer(buffer.buffer, buffer.size);
			sleep(1);
			
			if((buffer.counter+1) > buffer.size || (i==consume_num_itens-1)){ //libera a região crítica se a próxima inserção causa deadlock 
				inCriticRegion = false;										  //libera a região crítica se já produziu a quantidade necessária
				pthread_mutex_unlock(&mutex); // desbloqueia a região crítica
			}
			
			sem_post(&semaphore_full); // incrementa número de espaços utilizados no buffer
		}
		sleep((rand()%2)+1);
	}
}

void *consumerFun(void *args){

	int i, item;
	int inCriticRegion = false;

	while(true){

		for( i=0; i < consume_num_itens; i++ ){
	
			sem_wait(&semaphore_full); //se não tem itens suficientes, a thread fica bloqueada esperando o recurso

			if(!inCriticRegion){
				pthread_mutex_lock(&mutex); // bloqueia a região crítica para consumir o buffer
				inCriticRegion = true;
			}

			item = popItem(&buffer);
			printf("Consumed item [%d] - ", item);
			printBuffer(buffer.buffer, buffer.size);
			sleep(1);
			
			if( ((buffer.counter-1) < 0) || (i==consume_num_itens-1)) //libera a região crítica se a próxima remoção causa deadlock 
			{														  //libera a região crítica se já consumiu a quantidade necessária
				inCriticRegion = false;
				pthread_mutex_unlock(&mutex); // desbloqueia a região crítica
			}
			
			sem_post(&semaphore_empty); // incrementa número de espaços utilizados no buffer
		}
		sleep((rand()%2)+1);
	}
}

void pushItem(Buffer *buff, int item){

	buffer.buffer[buffer.counter] = item;
	buffer.counter += 1;
}

int popItem(Buffer *buff){

	int item;
	item = buffer.buffer[buffer.counter-1];
	buffer.buffer[buffer.counter-1] = 0;
	buffer.counter -= 1;

	return item;
}

int *creatBuffer(int size){

	int *buff = (int*) malloc(size * sizeof(int));
	int i;

	if(buff==NULL){
		perror("ERROR: Failed to create buffer...");
		exit(0);
	}
	for(i=0; i<size; i++){
		buff[i]=0;
	}

	return buff;
}

void printBuffer(int *vet, int size){

    int i;
    printf("buffer: [");
    for(i=0; i<size; i++){

    	if(vet[i]!=0){
    		printf("%d ", vet[i]);
    	}
    }
    printf("]\n\n");
}
