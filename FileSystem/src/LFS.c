#include "LFS.h"
#include "serializacion.h"
void* consola();
char* apiLissandra(char*);
char* selects(char* nombreTabla, u_int16_t key);
char* insert(char* nombreTabla, u_int16_t key, char* valor, double timeStamp);
char* create(char* nombreTabla, char* tipoConsistencia, u_int cantidadParticiones, u_int compactionTime);
char* describe(char* nombreTabla);
char* drop(char* nombreTabla);
bool existeTabla(char* nombreTabla);
FILE* obtenerMetaDataLectura(char* nombreTabla);
int funcionModulo(int key, int particiones);
int obtenerParticiones(FILE* metadata);
char* obtenerValue(char* nombreTabla, u_int16_t key, int particion);
FILE* obtenerBIN(int particion, char* nombreTabla);
void crearDirectiorioDeTabla(char* nombreTabla);
void crearMetadataDeTabla(char* nombreTabla, char* tipoConsistencia, u_int cantidadParticiones, u_int compactionTime);
void crearBinDeTabla(char* nombreTabla, int cantParticiones);
void dumpDeTablas(t_list *memTableAux);
void agregarAMemTable(char* nombreTabla, u_int16_t key, char* valor, double timeStamp);
uint64_t getTimestamp();

void* consola(){
	char *linea;
	char *resultado;
	while(1) {
		linea = readline(">");

		if(!strcmp(linea,"exit")){
			free(linea);
			break;
		}
		resultado = apiLissandra(linea);
		free(linea);
		puts(resultado);
		free(resultado);
	}
	return 0;
}

char *apiLissandra(char* mensaje){
	char** comando = string_split(mensaje, " ");
	if(comando[0]){
		u_int16_t cantArgumentos = 0;
		while(comando[cantArgumentos+1]){
			cantArgumentos++;
		}

		if(!strcmp(comando[0],"SELECT")){
			//SELECT [NOMBRE_TABLA] [KEY]
			//SELECT TABLA1 3
			free(comando[0]);
			if(cantArgumentos == 2){
				char* nombreTabla = comando[1];
				char* keystr = comando[2];
				char* endptr;
				ulong key = strtoul(keystr, &endptr, 10);
				if(*endptr == '\0'&& key < 65536){
					char* resultado = selects(nombreTabla, key);
					free(nombreTabla);
					free(keystr);
					free(comando);
					return resultado;
				}
			}
			while(cantArgumentos){
				free(comando[cantArgumentos]);
				cantArgumentos--;
			}
			free(comando);
			return string_from_format("Sintaxis invalida. Uso: SELECT [NOMBRE_TABLA] [KEY]");
		}
		else if(!strcmp(comando[0],"INSERT")){
			//INSERT [NOMBRE_TABLA] [KEY] “[VALUE]” [Timestamp]
			//INSERT TABLA1 3 "Mi nombre es Lissandra"
			//INSERT TABLA1 3 "Mi nombre es Lissandra" 1548421507

			free(comando[0]);

			if (cantArgumentos == 6) {//Si viene SIN timestamp
				char** argumentos = string_n_split(mensaje, 4, " ");
				char** ultimoArgumento = string_split(argumentos[3], "\""); //Sirve para el value
				free(argumentos[0]);
				free(argumentos[1]);
				free(argumentos[2]);
				free(argumentos[3]);
				free(argumentos);

				if(!ultimoArgumento[1]){ // Si hay un solo argumento sigo, sino es que hay argumentos de mas...
					char* nombreTabla = comando[1];
					char* keystr = comando[2];
					char* endptr;
					ulong key = strtoul(keystr, &endptr, 10);
					char* valor = ultimoArgumento[0];
					double timestamp = getTimestamp();

					if (*endptr == '\0' && key < 65536) {
						char* resultado = insert(nombreTabla, key, valor, timestamp);
						while(cantArgumentos){
							free(comando[cantArgumentos]);
							cantArgumentos--;
						}
						free(valor);
						free(ultimoArgumento);
						free(comando);
						return resultado;
					}
				}

				for(int i = 0; ultimoArgumento[i]; i++){
					free(ultimoArgumento[i]);
				}
				free(ultimoArgumento);
			}else if (cantArgumentos == 7) { // Si viene CON timestamp
				char** argumentos = string_n_split(mensaje, 5, " ");
				char** ultimoArgumento = string_split(argumentos[3], "\""); //Sirve para el value
				free(argumentos[0]);
				free(argumentos[1]);
				free(argumentos[2]);
				free(argumentos[3]);
				free(argumentos);

				if(!ultimoArgumento[1]){ // Si hay un solo argumento sigo, sino es que hay argumentos de mas...
					char* nombreTabla = comando[1];
					char* keystr = comando[2];
					char* endptr;
					ulong key = strtoul(keystr, &endptr, 10);
					char* valor = ultimoArgumento[0];
					char* timestampptr = comando[7];
					double timestamp = strtod(timestampptr, NULL);
					if (*endptr == '\0' && key < 65536) {
						char* resultado = insert(nombreTabla, key, valor, timestamp);
						while(cantArgumentos){
							free(comando[cantArgumentos]);
							cantArgumentos--;
						}
						free(valor);
						free(ultimoArgumento);
						free(comando);
						return resultado;
					}
				}

				for(int i = 0; ultimoArgumento[i]; i++){
					free(ultimoArgumento[i]);
				}
				free(ultimoArgumento);
			}

			while(cantArgumentos){
				free(comando[cantArgumentos]);
				cantArgumentos--;
			}
			free(comando);
			return string_from_format("Sintaxis invalida. Uso: INSERT [NOMBRE_TABLA] [KEY] “[VALUE]”");
		}
		else if(!strcmp(comando[0],"CREATE")){
			//CREATE [NOMBRE_TABLA] [TIPO_CONSISTENCIA] [NUMERO_PARTICIONES] [COMPACTION_TIME]
			//CREATE TABLA1 SC 4 60000

			free(comando[0]);
			if(cantArgumentos == 4){
				char* nombreTabla = comando[1];
				char* tipoConsistencia = comando[2];

				char* cantidadParticionesstr = comando[3];
				char* compactionTimestr = comando[4];
				char* endptr = 0;
				ulong cantidadParticiones = strtoul(cantidadParticionesstr, &endptr, 10);
				ulong compactionTime;
				if(*endptr == '\0')
					compactionTime = strtoul(compactionTimestr, &endptr, 10);
				if(*endptr == '\0'){
					// Faltaria revisar si el tipo de consistencia es valido ^
					char* resultado = create(nombreTabla, tipoConsistencia, cantidadParticiones, compactionTime);
					free(nombreTabla);
					free(tipoConsistencia);
					free(cantidadParticionesstr);
					free(compactionTimestr);
					free(comando);
					return resultado;
				}
			}
			while(cantArgumentos){
				free(comando[cantArgumentos]);
				cantArgumentos--;
			}
			free(comando);
			return string_from_format("Sintaxis invalida. Uso: CREATE [NOMBRE_TABLA] [TIPO_CONSISTENCIA] [NUMERO_PARTICIONES] [COMPACTION_TIME]");
		}
		else if(!strcmp(comando[0],"DESCRIBE")){
			//DESCRIBE [NOMBRE_TABLA]
			//DESCRIBE TABLA1
			free(comando[0]);
			if(cantArgumentos == 1){
				char* nombreTabla = comando[1];
				char* resultado = describe(nombreTabla);
				free(nombreTabla);
				free(comando);
				return resultado;
			} else if(cantArgumentos == 0){
				char* resultado = describe(NULL);
				return resultado;
			}
			while(cantArgumentos){
				free(comando[cantArgumentos]);
				cantArgumentos--;
			}
			free(comando);
			return string_from_format("Sintaxis invalida. Uso: DESCRIBE [NOMBRE_TABLA]");
		}
		else if(!strcmp(comando[0],"DROP")){
			//DROP [NOMBRE_TABLA]
			//DROP TABLA1

			free(comando[0]);
			if(cantArgumentos == 1){
				char* nombreTabla = comando[1];
				char* resultado = drop(nombreTabla);
				free(nombreTabla);
				free(comando);
				return resultado;
			}
			while(cantArgumentos){
				free(comando[cantArgumentos]);
				cantArgumentos--;
			}
			free(comando);
			return string_from_format("Sintaxis invalida. Uso: DROP [NOMBRE_TABLA]");
		}

		while(cantArgumentos){
			free(comando[cantArgumentos]);
			cantArgumentos--;
		}
		free(comando[0]);
	}
	free(comando);
	return string_from_format("Comando invalido");
}

char* selects(char* nombreTabla, u_int16_t key){
	char* valueReturn;
	if(existeTabla(nombreTabla)){
		FILE* metadata = obtenerMetaDataLectura(nombreTabla);
		int particiones = obtenerParticiones(metadata);
		printf("Obtuve la cantidad de particiones: %d \n", particiones);
		int particion = funcionModulo(key, particiones);
		printf("La Particion a la que hay que ir a buscar es: %d \n", particion);

//		obtenerRegistrosDeMemTable();
//		obtenerRegistrosDeTable();

		valueReturn = obtenerValue(nombreTabla, key, particion);

		fclose(metadata);
		log_debug(logger, "SELECT: Recibi Tabla: %s Key: %d", nombreTabla, key);
		return string_from_format("El value es: %s",valueReturn);
	}else{
		log_debug(logger, "No existe en el File System la tabla: %s",nombreTabla);
		return string_from_format(" No existe la Tabla: %s",nombreTabla);
	}

}

char* insert(char* nombreTabla, u_int16_t key, char* valor, double timeStamp){
	if(existeTabla(nombreTabla)){
		FILE* metadata = obtenerMetaDataLectura(nombreTabla); //Nose xq pide "obtener metadata"
		agregarAMemTable(nombreTabla, key, valor, timeStamp);

		fclose(metadata);
		log_debug(logger, "INSERT: Tabla: %s, Key: %d, Valor: %s, Timestamp: %f", nombreTabla, key, valor, timeStamp);
		return string_from_format("Se realizo el INSERT");
	}else{
		log_debug(logger, "No existe en el File System la tabla: %s",nombreTabla);
		return string_from_format("No existe en el File System la tabla: %s",nombreTabla);
	}


}

char* create(char* nombreTabla, char* tipoConsistencia, u_int cantidadParticiones, u_int compactionTime){
//	CREATE TABLA1 SC 4 60000
	if(!existeTabla(nombreTabla)){
		crearDirectiorioDeTabla(nombreTabla);
		crearMetadataDeTabla(nombreTabla, tipoConsistencia, cantidadParticiones, compactionTime);
		crearBinDeTabla(nombreTabla, cantidadParticiones);
		log_debug(logger, "CREATE: Recibi Tabla: %s TipoDeConsistencia: %s CantidadDeParticines: %d TiempoDeCompactacion: %d", nombreTabla, tipoConsistencia, cantidadParticiones, compactionTime);
	}else{
		log_info(logger, "La tabla %s ya existe en el FS", nombreTabla);
	}
	return string_from_format("Elegiste CREATE");
}

char* describe(char* nombreTabla){
	if(nombreTabla){
		log_debug(logger, "DESCRIBE: Recibi Tabla: %s", nombreTabla);
	}else{
		log_info(logger, "NO me pasaron una tabla con DESCRIBE");
	}

	return string_from_format("Elegiste DESCRIBE");
}

char* drop(char* nombreTabla){
	t_registro *imprimir;
	for(int i=0;i<cont;i++){
			imprimir = list_get(memTable,i);
			printf("Nombre de Tabla: %s \nKey: %u \nValue: %s \n", imprimir->nombre_tabla, imprimir->key, imprimir->value);
		}
		cont = 0;
	log_debug(logger, "DROP: Recibi Tabla: %s", nombreTabla);
	return string_from_format("Elegiste DROP");
}

//Descarga toda la informacion de la memtable, de todas las tablas, y copia dichos datos en los ditintos archivos temporales (uno por tabla). Luego se limpia la memtable.
void* dump(int tiempo_dump){
	int tiempo = tiempo_dump/1000;
	t_list* memTableAux = list_create();

	while(1){
		sleep(tiempo);
		if(!list_is_empty(memTable)){
			memTableAux = list_duplicate(memTable);
			list_destroy_and_destroy_elements(memTable, free); //Hago esto para que se puedan seguir efectuando insert sin complicar las cosas
			dumpDeTablas(memTableAux); //
			log_info(logger, "Dump realizado, numero: %d", cantDumps+1);
		}else{
			log_info(logger, "No se puede hacer Dump: Memtable vacía");
		}
	}


	return 0;
}

//Distribuye las disitntas Key dentro de dicha tabla. Se dividirá la key por la cantidad de particiones y el resto de la operación será la partición a utilizar
int funcionModulo(int key, int particiones){
	return key % particiones;
}

char* obtenerValue(char* nombreTabla, u_int16_t key, int particion){
	char* value = "Valor Default";
	int contador = 0;
	void *listaDeReg = list_create();
	t_registroBusqueda *reg = malloc(sizeof(t_registroBusqueda));
	FILE* bin = obtenerBIN(particion, nombreTabla);

	reg->key = 10;
	reg->timeStamp = 400;
	reg->value = "Value 1";
	fwrite(&reg, sizeof(t_registroBusqueda), 1, bin);
/*	reg->key = 10;
	reg->timeStamp = 100;
	reg->value = "Value 2";
	fwrite(&reg, sizeof(t_registroBusqueda), 1, bin);
	reg->key = 10;
	reg->timeStamp = 200;
	reg->value = "Value 3";
	fwrite(&reg, sizeof(t_registroBusqueda), 1, bin);
*/
	while(!feof(bin)){
		t_registroBusqueda *reg2;
		fread(&reg2, sizeof(t_registroBusqueda), 1, bin);
		if(reg->key == key){
			list_add(listaDeReg, reg2);
			contador++;
		}
	}
	printf("Contador = %d \n", contador);
	t_registroBusqueda *aux1;
	t_registroBusqueda *aux2;
	if(contador > 0){
		if(contador == 1){
			aux1 = list_get(listaDeReg,0);
			value = aux1->value;
		}else if(contador == 2){
			printf("Contador == 2");
			aux1 = list_get(listaDeReg,0);
			aux2 = list_get(listaDeReg,1);
			if(aux1->timeStamp > aux2->timeStamp){
				value = aux1->value;
			}else{
				value = aux2->value;
			}
		}else{
			aux1 = list_get(listaDeReg,0);
			aux2 = list_get(listaDeReg,1);
			for(int i=2;i<contador;i++){
				if(aux1->timeStamp > aux2->timeStamp){
					aux2 = list_get(listaDeReg,i);
				}else{
					aux1 = list_get(listaDeReg,i);
				}
			}
			if(aux1->timeStamp > aux2->timeStamp){
				value = aux1->value;
			}else{
				value = aux2->value;
			}

		}
	}else{
		printf("No hay registros con esa Key");
	}

	free(reg);
	fclose(bin);
	return value;
}
// ############### Extras ###############

t_config* leer_config() {
	return config_create("LFS.config");
}

t_log* iniciar_logger() {
	return log_create("Lissandra.log", "Lissandra", 1, LOG_LEVEL_TRACE);
}

void agregarAMemTable(char* nombreTabla, u_int16_t key, char* valor, double timeStamp){
	t_registro *registro = malloc(sizeof(t_registro));
	registro->nombre_tabla = malloc(strlen(nombreTabla)+1);
	registro->value = malloc(strlen(valor)+1);
	strcpy(registro->nombre_tabla, nombreTabla);
	registro->key = key;
	strcpy(registro->value, valor);
	registro->timeStamp = timeStamp;
	list_add(memTable, registro);
}

uint64_t getTimestamp() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}
