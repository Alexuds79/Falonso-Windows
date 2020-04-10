// falonso2.cpp : Un día en las carreras (de coches)
// Alejandro Mateos Pedraza y María Pérez Morales (todos los derechos reservados)

//#include "stdafx.h"
#include "pch.h"
#include "windows.h"
#include "falonso2.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define TAM_MEMORIA 600

#define LIBRE 1
#define OCUPADO 0
#define OCUPADO_SV 2
#define OCUPADO_SH 3
#define INF 999999

DWORD WINAPI funcionCoche(LPVOID);
DWORD WINAPI thirtySeconds(HANDLE);

struct globalVars {
	HINSTANCE dll;

	int numCoches;

	HANDLE coches[20];
	HANDLE cronometro;

	//Espacio comun
	int memoria[70];
	int contador = 0;


	//Semaforos
	HANDLE sb; //Participa en el banderazo
	HANDLE sc; //Seccion critica
	HANDLE sCoche[20]; //Un semaforo para cada coche

	//Eventos
	HANDLE banderazo;
	HANDLE semH, semV; //Espera en los semaforos fisicos
} gv;

//Funciones de falonso2.h de identificador global
int(*estado_semAforo)(int);
int(*posiciOn_ocupada)(int, int);

//Funciones propias
int mirarSemaforo(int posicion, int carril);
int tengoCocheDelante(int posicion, int carril);
int mirarAdelantar(int posicion, int carril);

//Función para imprimir el error en los parámetros iniciales de entrada
void printError(void) {
	printf("\n\nERROR: ./falonso2.c {numero de coches(max.20)} {velocidad(0/1)}\n\n");
	fflush(stdout);
}

int main(int argc, char *argv[]) {
	//Variables
	TCHAR username[20], dllPath[256];
	DWORD bufCharCount = 20;
	int modo;

	//--------------- Precondiciones --------------------//
	if (argc != 3) {
		printError();
		exit(1);
	}
	else {
		gv.numCoches = atoi(argv[1]);

		if (gv.numCoches < 1 || gv.numCoches>20) {
			printError();
			exit(1);
		}

		modo = atoi(argv[2]);

		if (strcmp(argv[2], "0") != 0 && strcmp(argv[2], "1") != 0) {
			printError();
			return 1;
		}
	}
	//------------------------------------------------//

	/******************** CARGA DE LA DLL *****************************/
	if (!GetUserName(username, &bufCharCount)) { PERROR("GetUserName"); exit(1); }
	sprintf_s(dllPath, "C:\\Users\\%s\\Desktop\\falonso2\\falonso2\\falonso2.dll", username);

	gv.dll = LoadLibrary(dllPath);
	if (gv.dll == NULL) { PERROR("LoadLibrary"); exit(1); }

	//Funciones
	int(*inicio_alonso)(int) = (int(*)(int))GetProcAddress(gv.dll, "FALONSO2_inicio");
	if (inicio_alonso == NULL) { PERROR("GetProcAddress:FALONSO2_inicio"); exit(1); }

	int(*luz_semAforo)(int, int) = (int(*)(int, int))GetProcAddress(gv.dll, "FALONSO2_luz_semAforo");
	if (luz_semAforo == NULL) { PERROR("GetProcAddress:FALONSO2_luz_semAforo"); exit(1); }

	//Pintamos el circuito
	if (inicio_alonso(modo) == -1) { PERROR("inicio_alonso:"); exit(1); }
	/******************************************************************/

	//Semaforos (inicializacion)
	gv.sb = CreateSemaphore(NULL, 0, gv.numCoches, NULL);
	gv.sc = CreateSemaphore(NULL, 1, 1, NULL);
	for (int i = 0; i < gv.numCoches; i++) gv.sCoche[i] = CreateSemaphore(NULL, 0, 1, NULL);

	//Eventos
	gv.banderazo = CreateEvent(NULL, TRUE, FALSE, LPCSTR("\rBanderazo inicial"));
	gv.semH = CreateEvent(NULL, TRUE, FALSE, LPCSTR("\rCambio del semaforo horizontal"));
	gv.semV = CreateEvent(NULL, TRUE, FALSE, LPCSTR("\rCambio del semaforo vertical"));

	//Creo los coches
	for (int i = 1; i <= gv.numCoches; i++) {
		gv.coches[i - 1] = CreateThread(NULL, NULL, funcionCoche, LPVOID(i), 0, NULL);
		WaitForSingleObject(gv.sb, INFINITE);
	}

	/*********** BANDERAZO!! ****************/
	SetEvent(gv.banderazo);
	/***************************************/

	//Comienzan los 30 sec (creamos un hijo que se dedique a contar...)
	gv.cronometro = CreateThread(NULL, NULL, thirtySeconds, NULL, 0, NULL);

	//Bucle de cambio del semaforo
	while (1) {
		if (luz_semAforo(HORIZONTAL, VERDE) == -1) { PERROR("luz_semaforo:"); exit(1); }
		if (luz_semAforo(VERTICAL, ROJO) == -1) { PERROR("luz_semaforo:"); exit(1); }

		SetEvent(gv.semH);
		Sleep(2000);

		if (luz_semAforo(HORIZONTAL, AMARILLO) == -1) { PERROR("luz_semaforo:"); exit(1); }

		Sleep(1000);

		if (luz_semAforo(HORIZONTAL, ROJO) == -1) { PERROR("luz_semaforo:"); exit(1); }
		if (luz_semAforo(VERTICAL, VERDE) == -1) { PERROR("luz_semaforo:"); exit(1); }

		SetEvent(gv.semV);
		Sleep(2000);

		if (luz_semAforo(VERTICAL, AMARILLO) == -1) { PERROR("luz_semaforo:"); exit(1); }

		Sleep(1000);
	}
}

DWORD WINAPI thirtySeconds(LPVOID) {

	if (WaitForMultipleObjects(gv.numCoches, gv.coches, FALSE, 30000) != WAIT_TIMEOUT) { PERROR("WaitForMultipleObjects:"); exit(1); }

	int(*fin_falonso)(int*) = (int(*)(int*))GetProcAddress(gv.dll, "FALONSO2_fin");
	if (fin_falonso == NULL) { PERROR("GetProcAddress:FALONSO2_fin"); exit(1); }

	if (fin_falonso(&(gv.contador)) == -1) { PERROR("fin_falonso:"); exit(1); }

	for (int i = 0; i < gv.numCoches; i++) CloseHandle(gv.coches[i]);
	for (int i = 0; i < gv.numCoches; i++) CloseHandle(gv.sCoche[i]);
	CloseHandle(gv.sb);
	CloseHandle(gv.sc);
	CloseHandle(gv.banderazo);
	CloseHandle(gv.semH);
	CloseHandle(gv.semV);
	CloseHandle(gv.cronometro);

	exit(0);
}

DWORD WINAPI funcionCoche(LPVOID i) {
	int hiloAvanza, posAux = 0, carrilAux = 0;
	int posConsulta, carrilConsulta, avisado;

	/************** DECLARACIÓN DE FUNCIONES *********************/
	//inicio_coche(&carril, &posicion, color);
	int(*inicio_coche)(int*, int*, int) = (int(*)(int*, int*, int))GetProcAddress(gv.dll, "FALONSO2_inicio_coche");
	if (inicio_coche == NULL) { PERROR("GetProcAddress:FALONSO2_inicio_coche"); exit(1); }

	//avance_coche(&carril, &posicion, color);
	int(*avance_coche)(int*, int*, int) = (int(*)(int*, int*, int))GetProcAddress(gv.dll, "FALONSO2_avance_coche");
	if (avance_coche == NULL) { PERROR("GetProcAddress:FALONSO2_avance_coche"); exit(1); }

	//velocidad(velocimetro, carril, posicion);
	int(*velocidad)(int, int, int) = (int(*)(int, int, int))GetProcAddress(gv.dll, "FALONSO2_velocidad");
	if (velocidad == NULL) { PERROR("GetProcAddress:FALONSO2_velocidad"); exit(1); }

	//estado_semAforo(direccion);
	estado_semAforo = (int(*)(int))GetProcAddress(gv.dll, "FALONSO2_estado_semAforo");
	if (estado_semAforo == NULL) { PERROR("GetProcAddress:FALONSO2_estado_semAforo"); exit(1); }

	//posiciOn_ocupada(carril, posicion);
	posiciOn_ocupada = (int(*)(int, int))GetProcAddress(gv.dll, "FALONSO2_posiciOn_ocupada");
	if (posiciOn_ocupada == NULL) { PERROR("GetProcAddress:FALONSO2_posiciOn_ocupada"); exit(1); }

	//cambio_carril(&carril, &posicion, color);
	int(*cambio_carril)(int*, int*, int) = (int(*)(int*, int*, int))GetProcAddress(gv.dll, "FALONSO2_cambio_carril");
	if (cambio_carril == NULL) { PERROR("GetProcAddress:FALONSO2_cambio_carril"); exit(1); }
	/*************************************************************/

	srand(GetCurrentThreadId());
	int carril = rand() % 2;
	int posicion = (int)i;
	int velocimetro = rand() % 98 + 1;

	int color;
	do {
		color = rand() % 8;
	} while (color == AZUL);

	//Guardo mi situacion en la memoria compartida
	gv.memoria[(int)i * 3 - 2] = INF; //posicion
	gv.memoria[(int)i * 3 - 1] = INF; //carril
	gv.memoria[(int)i * 3] = 0; //avisado

	if (inicio_coche(&carril, &posicion, color) == -1) { PERROR("inicio_coche:"); exit(1); }

	/******* Los coches esperan a que el padre haya dado la salida ******/
	ReleaseSemaphore(gv.sb, 1, NULL);
	WaitForSingleObject(gv.banderazo, INFINITE);
	/********************************************************************/

	while (1) {
		WaitForSingleObject(gv.sc, INFINITE);
		//Si el semaforo vertical está rojo, me paro...
		if (mirarSemaforo(posicion, carril) == OCUPADO_SV) {
			if (ReleaseSemaphore(gv.sc, 1, NULL) == 0) { PERROR("ReleaseSemafore:sc"); exit(1); };
			WaitForSingleObject(gv.semV, INFINITE);
		}

		//Si el semaforo horizontal está rojo, me paro...
		else if (mirarSemaforo(posicion, carril) == OCUPADO_SH) {
			if (ReleaseSemaphore(gv.sc, 1, NULL) == 0) { PERROR("ReleaseSemafore:sc"); exit(1); };
			WaitForSingleObject(gv.semH, INFINITE);
		}

		//Si el semaforo esta verde (o no estoy ante el) y no tengo nadie delante, avanzo
		else if (tengoCocheDelante(posicion, carril) == LIBRE) {
			if (avance_coche(&carril, &posicion, color) == -1) { PERROR("avance_coche:"); exit(1); }
			gv.memoria[(int)i * 3 - 2] = INF; //posicion
			gv.memoria[(int)i * 3 - 1] = INF; //carril
			gv.memoria[(int)i * 3] = 0; //avisado

			if ((carril == CARRIL_DERECHO && posicion == 133) || (carril == CARRIL_IZQUIERDO && posicion == 131)) {
				(gv.contador)++;
			}

			//Avisamos al coche que estaba esperando detrás de nosotros que puede avanzar
			/********************************************/
			for (int j = 1; j <= gv.numCoches; j++) {
				posConsulta = gv.memoria[(int)j * 3 - 2];
				carrilConsulta = gv.memoria[(int)j * 3 - 1];
				avisado = gv.memoria[(int)j * 3];

				if (posicion == 0) {
					if (posConsulta == 135 && carrilConsulta == carril) {
						if (avisado == 0) ReleaseSemaphore(gv.sCoche[j - 1], 1, NULL);
						gv.memoria[(int)j * 3]++; //avisado++
					}
				}

				else if (posicion == 1) {
					if (posConsulta == 136 && carrilConsulta == carril) {
						if (avisado == 0) ReleaseSemaphore(gv.sCoche[j - 1], 1, NULL);
						gv.memoria[(int)j * 3]++; //avisado++
					}
				}

				else if (posConsulta == posicion - 2 && carrilConsulta == carril) {
					if (avisado == 0) ReleaseSemaphore(gv.sCoche[j - 1], 1, NULL);
					gv.memoria[(int)j * 3]++; //avisado++
				}
			}
			/********************************************/

			//Avisamos al coche que estaba esperando justo fuera del cruce de que puede avanzar
			/********************************************/
			if (posicion == 23 + 1 && carril == CARRIL_IZQUIERDO) { posAux = 106 - 1; carrilAux = CARRIL_DERECHO; }
			if (posicion == 23 + 1 && carril == CARRIL_DERECHO) { posAux = 101 - 1; carrilAux = CARRIL_IZQUIERDO; }
			if (posicion == 21 + 1 && carril == CARRIL_DERECHO) { posAux = 108 - 1; carrilAux = CARRIL_DERECHO; }
			if (posicion == 25 + 1 && carril == CARRIL_IZQUIERDO) { posAux = 99 - 1; carrilAux = CARRIL_IZQUIERDO; }
			if (posicion == 106 + 1 && carril == CARRIL_DERECHO) { posAux = 23 - 1; carrilAux = CARRIL_IZQUIERDO; }
			if (posicion == 108 + 1 && carril == CARRIL_DERECHO) { posAux = 21 - 1; carrilAux = CARRIL_DERECHO; }
			if (posicion == 99 + 1 && carril == CARRIL_IZQUIERDO) { posAux = 25 - 1; carrilAux = CARRIL_IZQUIERDO; }
			if (posicion == 101 + 1 && carril == CARRIL_IZQUIERDO) { posAux = 23 - 1; carrilAux = CARRIL_DERECHO; }

			for (int j = 1; j <= gv.numCoches; j++) {
				posConsulta = gv.memoria[(int)j * 3 - 2];
				carrilConsulta = gv.memoria[(int)j * 3 - 1];
				avisado = gv.memoria[(int)j * 3];

				if (posConsulta == posAux && carrilConsulta == carrilAux) {
					if (avisado == 0) ReleaseSemaphore(gv.sCoche[j - 1], 1, NULL);
					gv.memoria[(int)j * 3]++; //avisado++
				}
			}
			/********************************************/

			ReleaseSemaphore(gv.sc, 1, NULL);
			if (velocidad(velocimetro, carril, posicion) == -1) { PERROR("velocidad:"); exit(1); }
		}

		else if ((hiloAvanza = tengoCocheDelante(posicion, carril)) > LIBRE) {
			switch (hiloAvanza) {
			case 105:
			case 107:
			case 98:
			case 100:
			case 221:
			case 20:
			case 24:
			case 220:
				gv.memoria[(int)i * 3 - 2] = posicion;
				gv.memoria[(int)i * 3 - 1] = carril;
				avisado = gv.memoria[(int)i * 3];

				if (ReleaseSemaphore(gv.sc, 1, NULL) == 0) { PERROR("ReleaseSemafore:sc"); exit(1); };

				//Esperar hasta que el coche que tengo delante me avise de que puedo avanzar
				if (avisado == 0) WaitForSingleObject(gv.sCoche[(int)i - 1], INFINITE);
			}
		}

		//Si estoy obstaculizado por delante...
		else if (tengoCocheDelante(posicion, carril) == OCUPADO) {

			//Si puedo adelantar, me cambio de carril...
			if (mirarAdelantar(posicion, carril)) {
				posAux = posicion - 1;
				carrilAux = carril;

				if (cambio_carril(&carril, &posicion, color) == -1) { PERROR("cambio_carril:"); exit(1); }

				gv.memoria[(int)i * 3 - 2] = INF; //posicion
				gv.memoria[(int)i * 3 - 1] = INF; //carril
				gv.memoria[(int)i * 3] = 0; //avisado

				//Avisamos al coche que estaba esperando detrás de nosotros que puede avanzar
				/********************************************/
				for (int j = 1; j <= gv.numCoches; j++) {
					posConsulta = gv.memoria[(int)j * 3 - 2];
					carrilConsulta = gv.memoria[(int)j * 3 - 1];
					avisado = gv.memoria[(int)j * 3];

					if (posConsulta == posAux && carrilConsulta == carrilAux) {
						if (avisado == 0) ReleaseSemaphore(gv.sCoche[j - 1], 1, NULL);
						gv.memoria[(int)j * 3]++; //avisado++
					}
				}
				/********************************************/

				if (ReleaseSemaphore(gv.sc, 1, NULL) == 0) { PERROR("ReleaseSemafore:sc"); exit(1); };
				if (velocidad(velocimetro, carril, posicion) == -1) { PERROR("velocidad:"); exit(1); }
			}

			//Si no puedo (obstaculizado totalmente), me paro...
			else {
				//Escribo donde estoy en la memoria compartida
				gv.memoria[(int)i * 3 - 2] = posicion;
				gv.memoria[(int)i * 3 - 1] = carril;
				avisado = gv.memoria[(int)i * 3];

				if (ReleaseSemaphore(gv.sc, 1, NULL) == 0) { PERROR("ReleaseSemafore:sc"); exit(1); };

				//Esperar hasta que el coche que tengo delante me avise de que puedo avanzar
				if (avisado == 0) WaitForSingleObject(gv.sCoche[(int)i - 1], INFINITE);
			}
		}
	}
}

int mirarSemaforo(int posicion, int carril) {
	//Las posiciones 20, 22, 105 y 98 son las posiciones inmediatamente anteriores a los ptos de comprobación de semáforo

	if (carril == CARRIL_DERECHO && posicion == 20) {
		if (estado_semAforo(VERTICAL) == VERDE) return LIBRE;
		else return OCUPADO_SV; //Semaforo vertical en rojo o amarillo
	}

	if (carril == CARRIL_IZQUIERDO && posicion == 22) {
		if (estado_semAforo(VERTICAL) == VERDE) return LIBRE;
		else return OCUPADO_SV;
	}

	if (carril == CARRIL_DERECHO && posicion == 105) {
		if (estado_semAforo(HORIZONTAL) == VERDE) return LIBRE;
		else return OCUPADO_SH; //Semaforo horizontal en rojo o amarillo
	}

	if (carril == CARRIL_IZQUIERDO && posicion == 98) {
		if (estado_semAforo(HORIZONTAL) == VERDE) return LIBRE;
		else return OCUPADO_SH;
	}

	return LIBRE; //Cuando consultamos el semáforo y estamos lejos de él, siempre recibimos LIBRE
}

int tengoCocheDelante(int posicion, int carril) {
	int posSig;

	if (posicion == 136) posicion = -1; //Circuito circular

	//Casos especiales del cruce (véase cuadro anexo)
	if (carril == CARRIL_DERECHO && posicion == (106 - 1) && posiciOn_ocupada(CARRIL_IZQUIERDO, 23)) return 105;
	if (carril == CARRIL_DERECHO && posicion == (108 - 1) && posiciOn_ocupada(CARRIL_DERECHO, 21)) return 107;
	if (carril == CARRIL_IZQUIERDO && posicion == (99 - 1) && posiciOn_ocupada(CARRIL_IZQUIERDO, 25)) return 98;
	if (carril == CARRIL_IZQUIERDO && posicion == (101 - 1) && posiciOn_ocupada(CARRIL_DERECHO, 23)) return 100;

	if (carril == CARRIL_IZQUIERDO && posicion == (23 - 1) && posiciOn_ocupada(CARRIL_DERECHO, 106)) return 221;
	if (carril == CARRIL_DERECHO && posicion == (21 - 1) && posiciOn_ocupada(CARRIL_DERECHO, 108)) return 20;
	if (carril == CARRIL_IZQUIERDO && posicion == (25 - 1) && posiciOn_ocupada(CARRIL_IZQUIERDO, 99)) return 24;
	if (carril == CARRIL_DERECHO && posicion == (23 - 1) && posiciOn_ocupada(CARRIL_IZQUIERDO, 101)) return 220;

	//Casos habituales
	posSig = posicion + 1;

	if (carril == CARRIL_IZQUIERDO && !posiciOn_ocupada(CARRIL_IZQUIERDO, posSig)) return LIBRE;
	else if (carril == CARRIL_DERECHO && !posiciOn_ocupada(CARRIL_DERECHO, posSig)) return LIBRE;
	else return OCUPADO;
}

int mirarAdelantar(int posicion, int carril) {
	//Si vamos a adelantar y justo vamos a dar a posición de comprobación de semáforo, cuando el semáforo está rojo, LO IMPEDIMOS
	//Las sumas y restas siguientes se justifican en la tabla de adelantamientos de la práctica (consúltese avellano)
	/**********************************/
	if ((estado_semAforo(HORIZONTAL) == ROJO || estado_semAforo(HORIZONTAL) == AMARILLO) && carril == CARRIL_DERECHO && posicion - 5 == 99) return 0;
	if ((estado_semAforo(HORIZONTAL) == ROJO || estado_semAforo(HORIZONTAL) == AMARILLO) && carril == CARRIL_IZQUIERDO && posicion + 5 == 106) return 0;
	if ((estado_semAforo(VERTICAL) == ROJO || estado_semAforo(VERTICAL) == AMARILLO) && carril == CARRIL_DERECHO && posicion + 1 == 23) return 0;
	if ((estado_semAforo(VERTICAL) == ROJO || estado_semAforo(VERTICAL) == AMARILLO) && carril == CARRIL_IZQUIERDO && posicion - 1 == 21) return 0;
	/**********************************/

	//Formalización de la tabla de equivalencias de adelantamientos
	//Una consulta de la forma [posicion+137] corresponde a una posición del carril izquierdo (véase la distribución de la memoria compartida)
	if (carril == CARRIL_DERECHO) {
		if (posicion >= 0 && posicion <= 13) { if (!posiciOn_ocupada(CARRIL_IZQUIERDO, posicion + 0)) return 1; }

		else if (posicion >= 14 && posicion <= 28) {
			//Si hay dos procesos en el cruce, no podemos adelantar si alguien de la horizontal ya está donde nosotros queremos ir
			//Ver cuadro anexo sobre posiciones equivalentes del cruce para entender las equivalencias 
			/****************************/
			if (posicion == 22 && (posiciOn_ocupada(CARRIL_IZQUIERDO, 23) || posiciOn_ocupada(CARRIL_DERECHO, 106))) return 0;
			if (posicion == 24 && (posiciOn_ocupada(CARRIL_IZQUIERDO, 25) || posiciOn_ocupada(CARRIL_IZQUIERDO, 99))) return 0;
			/****************************/

			if (!posiciOn_ocupada(CARRIL_IZQUIERDO, posicion + 1)) return 1;
		}

		else if (posicion >= 29 && posicion <= 60) { if (!posiciOn_ocupada(CARRIL_IZQUIERDO, posicion + 0))  return 1; }
		else if (posicion >= 61 && posicion <= 62) { if (!posiciOn_ocupada(CARRIL_IZQUIERDO, posicion - 1))  return 1; }
		else if (posicion >= 63 && posicion <= 65) { if (!posiciOn_ocupada(CARRIL_IZQUIERDO, posicion - 2))  return 1; }
		else if (posicion >= 66 && posicion <= 67) { if (!posiciOn_ocupada(CARRIL_IZQUIERDO, posicion - 3))  return 1; }
		else if (posicion == 68) { if (!posiciOn_ocupada(CARRIL_IZQUIERDO, posicion - 4))  return 1; }

		else if (posicion >= 69 && posicion <= 129) {
			/****************************/
			if (posicion == 106 && (posiciOn_ocupada(CARRIL_IZQUIERDO, 101) || posiciOn_ocupada(CARRIL_DERECHO, 23))) return 0;
			if (posicion == 104 && (posiciOn_ocupada(CARRIL_IZQUIERDO, 99) || posiciOn_ocupada(CARRIL_IZQUIERDO, 25))) return 0;
			/****************************/

			if (!posiciOn_ocupada(CARRIL_IZQUIERDO, posicion - 5))  return 1;
		}

		else if (posicion == 130) { if (!posiciOn_ocupada(CARRIL_IZQUIERDO, posicion - 3))  return 1; }
		else if (posicion >= 131 && posicion <= 134) { if (!posiciOn_ocupada(CARRIL_IZQUIERDO, posicion - 2))  return 1; }
		else if (posicion >= 135 && posicion <= 136) { if (!posiciOn_ocupada(CARRIL_IZQUIERDO, posicion - 1))  return 1; }

		return 0;
	}

	else if (carril == CARRIL_IZQUIERDO) {
		if (posicion >= 0 && posicion <= 15) { if (!posiciOn_ocupada(CARRIL_DERECHO, posicion + 0))  return 1; }

		else if (posicion >= 16 && posicion <= 28) {
			/****************************/
			if (posicion == 22 && (posiciOn_ocupada(CARRIL_DERECHO, 21) || posiciOn_ocupada(CARRIL_DERECHO, 108))) return 0;
			if (posicion == 24 && (posiciOn_ocupada(CARRIL_DERECHO, 23) || posiciOn_ocupada(CARRIL_IZQUIERDO, 101))) return 0;
			/****************************/

			if (!posiciOn_ocupada(CARRIL_DERECHO, posicion - 1))  return 1;
		}

		else if (posicion >= 29 && posicion <= 58) { if (!posiciOn_ocupada(CARRIL_DERECHO, posicion + 0))  return 1; }
		else if (posicion >= 59 && posicion <= 60) { if (!posiciOn_ocupada(CARRIL_DERECHO, posicion + 1))  return 1; }
		else if (posicion >= 61 && posicion <= 62) { if (!posiciOn_ocupada(CARRIL_DERECHO, posicion + 2))  return 1; }
		else if (posicion >= 63 && posicion <= 64) { if (!posiciOn_ocupada(CARRIL_DERECHO, posicion + 4))  return 1; }

		else if (posicion >= 65 && posicion <= 125) {
			/****************************/
			if (posicion == 101 && (posiciOn_ocupada(CARRIL_DERECHO, 106) || posiciOn_ocupada(CARRIL_IZQUIERDO, 23))) return 0;
			if (posicion == 103 && (posiciOn_ocupada(CARRIL_DERECHO, 108) || posiciOn_ocupada(CARRIL_DERECHO, 21))) return 0;
			/****************************/

			if (!posiciOn_ocupada(CARRIL_DERECHO, posicion + 5))  return 1;
		}

		else if (posicion == 126) { if (!posiciOn_ocupada(CARRIL_DERECHO, posicion + 4))  return 1; }
		else if (posicion >= 127 && posicion <= 128) { if (!posiciOn_ocupada(CARRIL_DERECHO, posicion + 3))  return 1; }
		else if (posicion >= 129 && posicion <= 133) { if (!posiciOn_ocupada(CARRIL_DERECHO, posicion + 2))  return 1; }
		else if (posicion >= 134 && posicion <= 136) { if (!posiciOn_ocupada(CARRIL_DERECHO, 136))  return 1; }

		return 0;
	}

	return 0;
}

