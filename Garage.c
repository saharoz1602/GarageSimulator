#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

typedef struct manager { //manager struct.
	int num_of_resources;
	int num_of_services;
	int num_of_cars;
} manager;

typedef struct resources { //for the resources struct array.
	int resource_type;
	char *resource_name;
	int stock;
} reso;

typedef struct service { //for the services struct array.
	int service_type;
	char *service_name;
	int service_time;
	int num_of_resources;
	int *resources_arr;
} servi;

typedef struct request { //for the requests struct array.
	int service_started;
	int car_id;
	int arrival_time;
	int num_of_services;
	int *services_list;
} requ;

int timer = 0;
sem_t mutex;

void res_init(char *filename);
void serv_init(char* filename);
void req_init(char *filename);
void*clock_set(void* x);
int  get_service_index(int x);
int  check_resource(int x);
void take_resource(int x);
void return_resource(int x);
void *car_handle(void *r_index);
void free_serivce();
void free_request();
void free_resource();

manager supervisor;
reso *resource;
servi *service;
requ *request;

int main(int argc, char* argv[]) {
	int i,*car_index_arr, services_start = 0;
	pthread_t time, *cars;

	if (argc != 4) {
		perror("wrong input!\n");
		exit(1);
	}
	req_init("requests.txt");
	res_init("resources.txt");
	serv_init("services.txt");

	sem_init(&mutex, 0, 1);

	pthread_create(&time, NULL, clock_set, NULL);//Initialize the timer.

	if ((cars = (pthread_t*)malloc(sizeof(pthread_t) * supervisor.num_of_cars)) == NULL)
		{
			perror("cars array memory alloc failed!\n");
			free_request();
			free_resource();
			free_serivce();
			exit(1);
		}

	if((car_index_arr = (int*) malloc(sizeof(int) * supervisor.num_of_cars))==NULL)
	{
		perror("car index memory alloc failed!\n");
		free_request();
		free_serivce();
		free_resource();
		exit(1);
	}

	for (i = 0; i < supervisor.num_of_cars; i++) {
		car_index_arr[i] = i;
	}

	while (services_start != supervisor.num_of_cars) {//running till the number of cars.
		for (i = 0; i < supervisor.num_of_cars; i++) {//check if car can enter to the garage.
			if ((timer >= request[i].arrival_time)&&(request[i].service_started == 0)) {//check if the arrival time is like the timer and let the car enter the garage.
				request[i].service_started = 1;
				printf("car:%d time:%d arrived.\n", request[i].car_id, timer);
				pthread_create(&cars[i], NULL, car_handle,(void*) &car_index_arr[i]);//simulate the cars.
				services_start++;
			}
		}
	}

	for (i = 0; i < supervisor.num_of_cars; i++) {//make sure there is no more request and we can finish the program.
		pthread_join(cars[i], NULL);
	}

	free(car_index_arr);
	free(cars);
	free_serivce();
	free_request();
	free_resource();

	return 0;
}

void res_init(char *filename) {//setting the resource into the resource struct array.
	int from, rbytes, count = 0;
	char buffer[1500], *token;

	if ((resource = (reso*) malloc(sizeof(reso))) == NULL) {
		perror("resource array memory alloc failed!\n");
		exit(1);
	}

	if ((from = open(filename, O_RDONLY)) == -1) {
		perror("open resource file failed!\n");
		free(resource);
		exit(1);;
	}
	if ((rbytes = read(from, buffer, 1500)) == -1) {
		perror("read resource file failed!\n");
		free(resource);
		close(from);
		exit(1);
	}
	buffer[rbytes] = '\0';
	token = strtok(buffer, "\n\t");
	while (token != '\0') {
		resource[count].resource_type = atoi(token);
		token = strtok(NULL, "\n\t");
		if((resource[count].resource_name = (char*) malloc(sizeof(char) * (strlen(token) + 1)))==NULL)
		{
			perror("resource name memory alloc failed!\n");
			free_resource();
			close(from);
			exit(1);
		}
		strcpy(resource[count].resource_name, token);
		token = strtok(NULL, "\n\t");
		resource[count].stock = atoi(token);
		token = strtok(NULL, "\n\t");
		count++;
		if (token != '\0') {
			if ((resource = (reso*) realloc(resource, (count + 1) * sizeof(reso))) == NULL) {
				perror("resource array memory alloc failed!\n");
				free_resource();
				close(from);
				exit(1);
			}
		}
	}
	supervisor.num_of_resources = count;//for the manager stock.
	close(from);
}

void serv_init(char* filename) {//setting the services into the services struct array.
	int from, rbytes, count = 0, i;
	char buffer[1500], *token;

	if ((service = (servi*) malloc(sizeof(servi))) == NULL) {
		perror("service array memory alloc failed!\n");
		exit(1);
	}

	if ((from = open(filename, O_RDONLY)) == -1) {
		perror("open service file failed!\n");
		free(service);
		exit(1);
	}
	if ((rbytes = read(from, buffer, 1500)) == -1) {
		perror("read service file failed!\n");
		free(service);
		close(from);
		exit(1);
	}
	buffer[rbytes] = '\0';
	token = strtok(buffer, "\n\t");
	while (token != '\0') {
		service[count].service_type = atoi(token);
		token = strtok(NULL, "\n\t");
		if((service[count].service_name = (char*) malloc(sizeof(char) * (strlen(token) + 1)))==NULL)
		{
			perror("service name memory alloc failed!\n");
			free_serivce();
			close(from);
			exit(1);
		}
		strcpy(service[count].service_name, token);
		token = strtok(NULL, "\n\t");
		service[count].service_time = atoi(token);
		token = strtok(NULL, "\n\t");
		service[count].num_of_resources = atoi(token);
		if((service[count].resources_arr = (int*) malloc(sizeof(service[count].num_of_resources)))==NULL)
		{
			perror("array of resource memory alloc failed!\n");
			free_serivce();
			close(from);
			exit(1);
		}
		token = strtok(NULL, "\n\t");
		if (service[count].num_of_resources == 0) {
			token = strtok(NULL, "\n\t");
		} else {
			for (i = 0; i < service[count].num_of_resources; i++) {
				service[count].resources_arr[i] = atoi(token);
				token = strtok(NULL, "\n\t");
			}
		}
		count++;
		if (token != '\0') {
			if ((service = (servi*) realloc(service, (count + 1) * sizeof(servi)))
					== NULL) {
				perror("service array memory alloc realloc failed!\n");
				free_serivce();
				exit(1);
			}
		}
	}
	supervisor.num_of_services = count;
	close(from);
}

void req_init(char *filename) {
	int from, rbytes, count = 0, i;
	char buffer[1500], *token;

	if ((request = (requ*) malloc(sizeof(requ))) == NULL) {
		perror("requests array memory alloc failed!\n");
		exit(1);
	}

	if ((from = open(filename, O_RDONLY)) == -1) {
		perror("open requests file failed!\n");
		free(request);
		exit(1);
	}
	if ((rbytes = read(from, buffer, 1500)) == -1) {
		perror("read requests file failed!\n");
		free(request);
		close(from);
		exit(1);
	}
	buffer[rbytes] = '\0';
	token = strtok(buffer, "\n\t");
	while (token != '\0') {
		request[count].service_started = 0;
		request[count].car_id=atoi(token);
		token = strtok(NULL, "\n\t");
		request[count].arrival_time = atoi(token);
		token = strtok(NULL, "\n\t");
		request[count].num_of_services = atoi(token);
		token = strtok(NULL, "\n\t");
		if((request[count].services_list = (int*) malloc(sizeof(request[count].num_of_services)))==NULL)
		{
			perror("request service list memory alloc failed!\n");
			free_request();
			close(from);
			exit(1);
		}
		for (i = 0; i < request[count].num_of_services; i++) {
			request[count].services_list[i] = atoi(token);
			token = strtok(NULL, "\n\t");
		}
		if (request[count].num_of_services == 0) {
			token = strtok(NULL, "\n\t");
		}
		count++;
		if (token != '\0') {
			if ((request = (requ*) realloc(request, (count + 1) * sizeof(requ))) == NULL) {
				perror("requests array memory alloc realloc failed!\n");
				free_request();
				close(from);
				exit(1);
			}
		}
	}
	supervisor.num_of_cars = count;
	close(from);
}

void* clock_set(void* x){//for the timer to run.
	while (1) {
		sleep(1);
		timer++;
	}
}

int get_service_index(int x) {//function to check the type of service,return the index of the service.
	int i;
	for (i = 0; i < supervisor.num_of_services; i++) {
		if (service[i].service_type == x) {
			return i;
		}
	}
	return -1; //service doesn't exits
}

int check_resource(int x) {//function to check if the amount of the needed resource is available.
	int i;
	for (i = 0; i < supervisor.num_of_resources; i++) {
		if (resource[i].resource_type == x) {
			if (resource[i].stock > 0) {
				return 1;
			}
		}
	}
	return 0;
}

void take_resource(int x) {//function that decreased the resource stock if the service need it.
	int i;
	for (i = 0; i < supervisor.num_of_resources; i++) {
		if (resource[i].resource_type == x) {
			resource[i].stock--;
			return;
		}
	}
}

void return_resource(int x) {//function that increase the resource stock if the service is done
	int i;
	for (i = 0; i < supervisor.num_of_resources; i++) {
		if (resource[i].resource_type == x) {
			resource[i].stock++;
			return;
		}
	}
}

void free_serivce() {//function to free the services struct array.
	int i;
	for (i = 0; i < supervisor.num_of_services; i++) {
		free(service[i].service_name);
		free(service[i].resources_arr);
	}
	free(service);
}

void free_request() {//function to free the request struct array.
	int i;
	for (i = 0; i < supervisor.num_of_cars; i++) {
		free(request[i].services_list);
	}
	free(request);
}

void free_resource() {//function to free the resource struct array.
	int i;
	for (i = 0; i < supervisor.num_of_resources; i++) {
		free(resource[i].resource_name);
	}
	free(resource);
}

void *car_handle(void *r_index) {//this function start the services needed for every car
	int index = *(int*) r_index;

	int count = request[index].num_of_services;//to count the number of requests.

	printf("car:%d time:%d service request started.\n", request[index].car_id,timer);

	while (count > 0) {
		int i,j,serv_index,available_service;
		for (i = 0; i < request[index].num_of_services; i++) {//running on all the services the car need.
			if (request[index].services_list[i] != -1) {
				serv_index = get_service_index(request[index].services_list[i]);//save the index of the service.
				if (serv_index == (-1)) {//if the service doesn't exits.
					printf("service id:%d doesn't exits!\n",request[index].services_list[i]);
					request[index].services_list[i] = -1;
					count--;
				}
				sem_wait(&mutex);//try to take a resource.
				available_service = 0;
				for (j = 0; j < service[serv_index].num_of_resources; j++) {
					if (check_resource(service[serv_index].resources_arr[j])) {
						available_service++;
					}
				}
				if (available_service == service[serv_index].num_of_resources) {//check if we can start the service.
					printf("car:%d time:%d started %s.\n", request[index].car_id,timer, service[serv_index].service_name);
					for (j = 0; j < service[serv_index].num_of_resources; j++) {//to take the needed resource.
						take_resource(service[serv_index].resources_arr[j]);
					}
					sem_post(&mutex);//let others requests take resource while we do the service.
					sleep(service[serv_index].service_time);//sleep due the time of the service(critical code part).
					sem_wait(&mutex);//return the resource (finish the service).
					for (j = 0; j < service[serv_index].num_of_resources; j++) {//return all resources.
						return_resource(service[serv_index].resources_arr[j]);
					}
					printf("car:%d time:%d complete %s.\n", request[index].car_id,timer, service[serv_index].service_name);
					request[index].services_list[i] = -1;//to mark that this service done.
					count--;
				}
				sem_post(&mutex);//finish the critical code part (service is done).
			}
		}
	}
	printf("car:%d time:%d service complete.\n", request[index].car_id, timer);
	return NULL;
}
