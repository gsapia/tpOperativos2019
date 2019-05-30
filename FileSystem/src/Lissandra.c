/*
-Es el punto de entrada al LFS, administra la conexión con los demás módulos y  entiende como resolver los distintos pedidos que se le soliciten.

##### TABLAS #####
-Se divide a las tablas en particiones. Cada particion es un archivo en el FS. Cant de particiones? Definido en el Metadata.
-Cada tabla estara representada con un directorio, que contiene todos los archivos metadata (que corresponden a c/u de las particiones), ademas de temer un archivo de Metadata (propio)
-Tipos de archivos: (.bin - .tmp - tmpc


##### METADATA #####
-Contiene la informacion administrativa de la tabla.
	-Tipo de consistencia
	-Numero de Particiones
	-Tiempo entre compactaciones.

##### REGISTRO #####
-Los datos de la tabla estarán formados por registros, donde cada uno de ellos estará compuesto por 3 partes:
	-Timestamp (tiempo de la insercion de la key)
	-Key ( ... la key)
	-Value ( Con valor maximo en .config, tiene que controlar que no se hagan insert de tamaño mayor al maximo.)

¡Para la distribución de las distintas Key dentro de dicha tabla se aplicará una función módulo!
¡Se dividirá la key por la cantidad de particiones y el resto de la operación será la partición a utilizar!

##### .TMP / DUMPS / MEMTABLE
-.tmp = se guardan los datos que vayan modificandose para luego compactarlos
-Dos jerarquias de .tmp:
	-El area de "memtable", dividido por tabla, son los datos que no fueron dumpeados en un archivo temporal. Son los Insert que se acumulan hasta que se haga un DUMP.
	-Datos en .tmp que estan pendientes de compactacion

*/

#include "LFS.h"

void *consola();
t_config *leer_config();
t_log* iniciar_logger();
void* servidor(uint16_t puerto_escucha, int tamValue);
void* fileSystem(t_config *config);
void* dump(int tiempo_dump);

int main(void){
	memTable = list_create(); cont = 0; cantDumps = 0;
	config = leer_config();
	puntoMontaje = config_get_string_value(config,"PUNTOMONTAJE");
	int tiempo_dump = config_get_int_value(config, "TIEMPODUMP");
	int puerto_escucha = config_get_int_value(config, "PUERTOESCUCHA");
	int tamValue = config_get_int_value(config, "TAMAÑOVALUE");
	logger = iniciar_logger();
	log_info(logger, "Hola, soy Lissandra");

//	Servidor
/*	pthread_t hiloServidor;
	if(pthread_create(&hiloServidor, NULL, servidor, (puerto_escucha, tamValue))){
		log_error(logger, "Hilo servidor: Error - pthread_create()");
		exit(EXIT_FAILURE);
	}
*/

	//Creo las Tablas del FileSystem necesarias
	pthread_t hiloFS;
	if(pthread_create(&hiloFS, NULL, fileSystem, NULL)){
		log_error(logger, "Hilo FileSystem: Error - pthread_create()");
		exit(EXIT_FAILURE);
	}
	pthread_join(hiloFS,NULL);

	//Api Lissandra
	pthread_t hiloConsola;
	if(pthread_create(&hiloConsola, NULL, consola, NULL)){
		log_error(logger, "Hilo consola: Error - pthread_create()");
		exit(EXIT_FAILURE);
	}
/*
	pthread_t hiloDump;
	if(pthread_create(&hiloDump, NULL, dump, tiempo_dump)){
		log_error(logger, "Hilo Dump: Error - pthread_create()");
		exit(EXIT_FAILURE);
	}
*/
	pthread_join(hiloConsola, NULL);
//	pthread_join(hiloServidor,NULL);
//	pthread_join(hiloDump, NULL);
	log_destroy(logger);


	return EXIT_SUCCESS;
}

