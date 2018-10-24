// Compilación: g++ -std=c++11 -pthread nprods_ncons_FIFO.cpp -o nprods_ncons_FIFO Semaphore.cpp

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include <future>
#include <atomic>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//**********************************************************************
// variables compartidas

const int num_items = 75 ,   // número de items
	       	tam_vec   = 15 ;   // tamaño del buffer
int				primera_celda_ocupada_FIFO= 0, // primera celda ocupada en el vector. Se incrementa al leer
					primera_celda_libre= 0; //se incrementa al leer
unsigned  cont_prod[num_items] = {0}, // contadores de verificación: producidos
					buffer[tam_vec]= {0},
          cont_cons[num_items] = {0}; // contadores de verificación: consumidos
Semaphore puede_consumir= 0;	//número de entradas ocupadas del búffer (#E - #L)
Semaphore puede_producir= tam_vec; //número de entradas libres del búffer (k + #L - #E), con k el tamaño fijo del búffer
mutex mtx;
atomic<int> producidos, consumidos;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio() {
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(){
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

	 mtx.lock();
   cout << "producido: " << contador << endl << flush ;
	 mtx.unlock();

   cont_prod[contador] ++ ;
   return contador++ ;
}

//----------------------------------------------------------------------

void mostrar_buffer(){
	for(unsigned i= 0; i< tam_vec; i++)
		cout <<  " " << buffer[i];

	cout << endl;
}

//----------------------------------------------------------------------

void consumir_dato( unsigned dato ){
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

	 mtx.lock();
   cout << "                  consumido: " << dato << ".Buffer: " ;
	 mostrar_buffer();
	 mtx.unlock();
}


//----------------------------------------------------------------------

void test_contadores(){
	bool ok = true ;
	cout << "comprobando contadores ...." ;

	for( unsigned i = 0 ; i < num_items ; i++ ){
		if ( cont_prod[i] != 1 ){
			cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
			ok = false ;
		}

		if ( cont_cons[i] != 1 ){
			cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
			ok = false ;
		}
	}

	if (ok)
		cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora_FIFO(){
	int a;

	while(producidos < num_items){
		producidos++;

		sem_wait(puede_producir); //esperamos a que pueda escribir

		//cout << "\nEscribimos " << a << ". Buffer antes de escribir: ";
		//mostrar_buffer();
		a = producir_dato() ; //creamos el dato
		buffer[primera_celda_libre % tam_vec]= a; // lo escribimos en la primera posición libre del buffer (teniendo en cuenta que lo recorremos de manera circular)
		primera_celda_libre++;

		//cout << "\nBuffer después de escribir: ";
		//mostrar_buffer();
		sem_signal(puede_consumir); //indicamos que ya se puede leer
	}
	cout << "\nFin de hebra productora\n";
}


//----------------------------------------------------------------------

void funcion_hebra_consumidora_FIFO(){
	int b;
	while(consumidos < num_items){
		consumidos++;

		sem_wait(puede_consumir);
		//cout << "\nVamos a leer " << b << ". Buffer antes de leer: ";
		//mostrar_buffer();
		b= buffer[primera_celda_ocupada_FIFO % tam_vec];
		primera_celda_ocupada_FIFO++;
		consumir_dato(b) ;

		//cout << "\nBuffer después de leer: ";
		//mostrar_buffer();
		sem_signal(puede_producir);

		//sem_signal(alguien_consumiendo);
	}
	cout << "\nFin de hebra consumidora\n";
}
//----------------------------------------------------------------------

int main(){
	int n, prods, cons;
	producidos= consumidos= 0;

	cout << "--------------------------------------------------------" << endl
			 << "Problema de n productores- n consumidores (solución FIFO)." << endl
			 << "--------------------------------------------------------" << endl
		   << flush ;
	cout << "\nIntroduzca número deseado de productores y consumidores: ";
	cin >> prods >> cons;

	thread productores[prods], consumidores[cons];

	for(unsigned i= 0; i< prods; i++)
		productores[i]= thread(funcion_hebra_productora_FIFO);

	for(unsigned i= 0; i< cons; i++)
		consumidores[i]= thread(funcion_hebra_consumidora_FIFO);

	for(unsigned i= 0; i< prods; i++)
		productores[i].join();

	for(unsigned i= 0; i< cons; i++)
		consumidores[i].join();


	test_contadores();
}
