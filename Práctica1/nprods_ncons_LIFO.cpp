// Para compilar: g++ -std=c++11 -pthread nprods_ncons_LIFO.cpp -o nprods_ncons_LIFO Semaphore.cpp

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//**********************************************************************
// variables compartidas

const int num_items = 75 ,   // número de items
	       	tam_vec   = 15 ;   // tamaño del buffer
int				producidos= 0, consumidos= 0,
          primera_celda_ocupada_LIFO= 0; //se incrementa al leer
unsigned  cont_prod[num_items] = {0}, // contadores de verificación: producidos
					buffer[tam_vec]= {0},
          cont_cons[num_items] = {0}; // contadores de verificación: consumidos
Semaphore puede_consumir= 0;	//número de entradas ocupadas del búffer (#E - #L)
Semaphore puede_producir= tam_vec; //número de entradas libres del búffer (k + #L - #E), con k el tamaño fijo del búffer
mutex mtx;

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
	cout << buffer[i] << " ";
}

void consumir_dato( unsigned dato ){
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

	 mtx.lock();
   cout << "                  consumido: " << dato << endl ;
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

void  funcion_hebra_productora_LIFO(){
	int a;
	while(producidos < num_items){
    mtx.lock();
    producidos++;
    mtx.unlock();

	  sem_wait(puede_producir); //esperamos a que pueda escribir

    a = producir_dato() ;
		//escribimos el dato en a, sentencia e

    mtx.lock();
		buffer[primera_celda_ocupada_LIFO]= a;
		primera_celda_ocupada_LIFO++;
		mtx.unlock();

		sem_signal(puede_consumir); //indicamos que ya se puede leer
	}
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora_LIFO(){
	int b;
  while(consumidos < num_items){
    mtx.lock();
    consumidos++;
    mtx.unlock();

    sem_wait(puede_consumir);
		//leemos b del buffer, sentencia l
    mtx.lock();
    primera_celda_ocupada_LIFO--;
		b= buffer[primera_celda_ocupada_LIFO];
		mtx.unlock();

    consumir_dato(b) ;
		sem_signal(puede_producir);
	}
}

//----------------------------------------------------------------------

int main(){
  cout << "--------------------------------------------------------" << endl
        << "Problema de los n productores- n consumidores (solución LIFO)." << endl
        << "--------------------------------------------------------" << endl
        << flush ;
  int prods, cons;
  cout << "\nIntroduzca número deseado de productores y consumidores: ";
  cin >> prods >> cons;

  thread productores[prods], consumidores[cons];

  for(int i= 0; i< prods; i++)
    productores[i]= thread(funcion_hebra_productora_LIFO);

  for(int i= 0; i< cons; i++)
    consumidores[i]= thread(funcion_hebra_consumidora_LIFO);

  for(int i= 0; i< prods; i++)
    productores[i].join() ;

  for(int i= 0; i< cons; i++)
    consumidores[i].join() ;

  test_contadores();
}
