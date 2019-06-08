#include "LFS.h"
int renombrarArchivosTemporales(char* path, int particiones);
void analizarTmpc(char* path, char* nombre_tabla, int diferencia);
int obtenerParticion(char* nombreTabla, u_int16_t key);
void discriminadorDeCasos(char* path, uint16_t key, uint64_t timestamp, char* value, int particion);
char* intToString(long a);
void casoParticular(char* path, int particion, FILE* bin, char* line, int renglon);
FILE* obtenerMetaDataLectura(char* nombreTabla);


void *compactacion(argumentos_compactacion *args){
	char* nombreTabla = malloc(strlen(args->nombreTabla));
	strcpy(nombreTabla, args->nombreTabla);
	int tiempo = args->compactation_time/1000;
	free(args->nombreTabla); free(args);
	FILE* metadata = obtenerMetaDataLectura(nombreTabla);

	int i = 0;
	size_t buffer_size = 0; char* buffer = NULL;
	while(getline(&buffer, &buffer_size, metadata) != -1 && i<1){
		free(buffer);
		buffer = NULL;
		i++;
	}
	char** linea= string_split(buffer, "=");
	free(buffer);
	int particiones = atoi(linea[1]) + 1;
	free(linea[0]); free(linea[1]); free(linea);

	while(1){
		sleep(tiempo);
		char* path = string_from_format("%sTable/%s", puntoMontaje, nombreTabla);
		int diferencia = renombrarArchivosTemporales(path, particiones);
		if(diferencia > 0){
			analizarTmpc(path, nombreTabla, diferencia);
			log_info(logger, "%s realizo compactacion de %d .tmpc", nombreTabla, diferencia);
		}
		free(path);
	}
}

int renombrarArchivosTemporales(char* path, int particiones){
	int i = 0; int cantArchivos = 0;
	char* path_aux = string_from_format("%s/A%d.tmp", path, i);
	char* pathAux;
	DIR* path_buscado = opendir(path);
	struct dirent* carpeta = readdir(path_buscado);

	while(carpeta){
		if(strcmp(carpeta->d_name, ".") && strcmp(carpeta->d_name, "..")){
			cantArchivos++;
		}
		carpeta = readdir(path_buscado);
	}
	int diferencia = cantArchivos - particiones;
	int diferenciaReturn = diferencia;
	while(diferencia != 0){
		if(access(path_aux, F_OK) != -1){//Existe el archivo .tmp
			pathAux = string_from_format("%sc", path_aux);
			rename(path_aux, pathAux);
			free(pathAux);
			diferencia--;
		}
		i++;
		free(path_aux);
		path_aux = string_from_format("%s/A%d.tmp", path, i);
	}

	free(path_aux);
	closedir(path_buscado);
	return diferenciaReturn;
}

void analizarTmpc(char* path, char* nombre_tabla, int diferencia){
	int i = 0;
	char* pathTempc = string_from_format("%s/A%d.tmpc", path, i);
	FILE* tempc;
	int particion;
	while(diferencia != 0){
		if(access(pathTempc, F_OK) != -1){		//Si podes acceder al .tmpc
			tempc = fopen(pathTempc, "r");						//Abro el .tmpc
			free(pathTempc);
			size_t buffer_size = 0;
			char* buffer = NULL;
			while(getline(&buffer, &buffer_size, tempc) != -1){ 				//Mientras existan lineas/renglones en el .tmpc
				char** linea = string_split(buffer, ";"); 						//Dividi el renglon
				char* endptrKey, *keystr = linea[1];
				ulong key_buffer = strtoul(keystr, &endptrKey, 10);

				char* timestampstr = linea[0], *endptrTimestamp;
				ulong timestamp_buffer = strtoul(timestampstr, &endptrTimestamp, 10);

				particion = obtenerParticion(nombre_tabla, key_buffer); 		//Con la key del .tmpc, busco la particion .bin

				discriminadorDeCasos(path, key_buffer, timestamp_buffer, linea[2], particion);
				free(linea[0]);
				free(linea[1]);
				free(linea[2]);
				free(linea);
				free(buffer);
				buffer = NULL;
			}
			free(buffer);
			fclose(tempc);
			pathTempc = string_from_format("%s/A%d.tmpc", path, i);
			remove(pathTempc);
			diferencia--;
		}
		free(pathTempc);
		i++;
		pathTempc = string_from_format("%s/A%d.tmpc", path, i);
	}
	free(pathTempc);
}

void discriminadorDeCasos(char* path, uint16_t key, uint64_t timestamp, char* value, int particion){
	char* pathBin = string_from_format("%s/%d.bin", path, particion);
	FILE* bin = fopen(pathBin, "r+");
	size_t buffer_size = 0;
	char* buffer = NULL;
	bool encontrado = false;
	int renglon = 0;

	// [TIMESTAMP;KEY;VALUE]
	while(getline(&buffer, &buffer_size, bin) > 0){
		renglon++;
		char **linea = string_split(buffer, ";");
		char* endptrKey, *keystr = linea[1];
		ulong key_buffer = strtoul(keystr, &endptrKey, 10);
		if(key == key_buffer){
			encontrado = true;
			char* timestampstr = linea[0];char* endptrTimestamp;

			ulong timestamp_buffer = strtoul(timestampstr, &endptrTimestamp, 10);

			if(timestamp > timestamp_buffer){ //Si el timestamp del .tempc es mayor al del .bin, reemplazo
				char* timestampC = intToString(timestamp);
				char* keyC = intToString(key);
				char* line = string_from_format("%s;%s;%s", timestampC, keyC, value);
				casoParticular(path, particion, bin, line, renglon);
				log_trace(logger, "Salgo");
				free(timestampC);
				free(keyC);
				free(line);
			}
			free(linea[0]);
			free(linea[1]);
			free(linea[2]);
			free(linea);
			break;
		}
		free(linea[0]);
		free(linea[1]);
		free(linea[2]);
		free(linea);
		free(buffer);
		buffer = NULL;
	}

	if(!encontrado){ //Si no se encontro, lo escribo al final
		char* timestampC = intToString(timestamp);
		char* keyC = intToString(key);
		char* line = string_from_format("%s;%s;%s", timestampC, keyC, value);
		fseek(bin, 0, SEEK_END);

		fputs(line, bin);
		free(timestampC);
		free(keyC);
		free(line);
	}
	free(pathBin);
	free(buffer);
	fclose(bin);
}

void casoParticular(char* path, int particion, FILE* bin, char* line, int renglon){
	char* nameAux = string_from_format("%s/%d.bin", path, bin);
	char* nameAux2 =string_from_format("%s/%AuxiliarTemporal.bin", path, bin);
	rename(nameAux, nameAux2);
	free(nameAux);
	char* pathBin = string_from_format("%s/%d.bin2", path, particion);
	FILE* f = fopen(pathBin, "w");
	int i = 0;

	size_t buffer_size = 80;
	char* buffer = malloc(buffer_size * sizeof(char));
	log_trace(logger, "FLAG_1");
	fseek(bin, 0, SEEK_SET);
	while(getline(&buffer, &buffer_size, bin) != -1){
		if(i==renglon){
			log_trace(logger, "Renglon: %d", renglon);
			fputs(line, f);
		}else{
			log_trace(logger, "FLAG_buffer");
			char **linea = string_split(buffer, ";");
			char* buffer_w = string_from_format("%s;%s%;%s", linea[0], linea[1], linea[2]);
			fputs(buffer_w, f);
			free(buffer_w);
			free(linea);
		}
		i++;
	}

	free(buffer);
	free(pathBin);
	fclose(bin);
	fclose(f);
	remove(nameAux2);
	free(nameAux2);
}
