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
int				primera_celda_ocupada_FIFO= 0, // primera celda ocupada en el vector. Se incrementa al leer
					primera_celda_libre= 0, //se incrementa al leer
					primera_celda_ocupada_LIFO= 0; // se incrementa al leer y se decrementa al escribir
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

void mostrar_buffer(){
	for(unsigned i= 0; i< tam_vec; i++)
		cout << buffer[i] << " ";
}

//----------------------------------------------------------------------

void  funcion_hebra_productora_LIFO(){
	int a;
	for( unsigned i = 0 ; i < num_items ; i++ ){
	  a = producir_dato() ;
	  sem_wait(puede_producir); //esperamos a que pueda escribir
		//escribimos el dato en a, sentencia e
		cout << "\nEscribimos " << a << ".Buffer antes de escribir: ";
		mostrar_buffer();

		buffer[primera_celda_ocupada_LIFO]= a;
		primera_celda_ocupada_LIFO++;

		cout << "\nBuffer después: ";
		mostrar_buffer();

		sem_signal(puede_consumir); //indicamos que ya se puede leer
	}
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora_LIFO(){
	int b;
	for( unsigned i = 0 ; i < num_items ; i++ ){
		sem_wait(puede_consumir);
		//leemos b del buffer, sentencia l
		cout << "\nLeemos " << b << ".Buffer antes de leer: ";
		mostrar_buffer();

		b= buffer[primera_celda_ocupada_LIFO];
		primera_celda_ocupada_LIFO--;

		cout << "\nBuffer después: ";
		mostrar_buffer();

		sem_signal(puede_producir);
		consumir_dato(b) ;
	}
}


//----------------------------------------------------------------------

void  funcion_hebra_productora_FIFO(){
	int a;
	for( unsigned i = 0 ; i < num_items ; i++ ){
	  a = producir_dato() ;
	  sem_wait(puede_producir); //esperamos a que pueda escribir

		cout << "\nEscribimos " << a << ". Buffer antes de escribir: ";
		mostrar_buffer();

		buffer[primera_celda_libre]= a;
		primera_celda_libre= (primera_celda_libre + 1) % tam_vec;

		cout << "\nBuffer después de escribir: ";
		mostrar_buffer();

		sem_signal(puede_consumir); //indicamos que ya se puede leer
	}
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora_FIFO(){
	int b;
	for( unsigned i = 0 ; i < num_items ; i++ ){
		sem_wait(puede_consumir);
		//leer a de vec, sentencia l
		cout << "\nVamos a leer " << b << ". Buffer antes de leer: ";
		mostrar_buffer();

		b= buffer[primera_celda_ocupada_FIFO];
		primera_celda_ocupada_FIFO++;

		cout << "\nBuffer después de leer: ";
		mostrar_buffer();

		sem_signal(puede_producir);
		consumir_dato(b) ;
	}
}
//----------------------------------------------------------------------

int main(){
   cout << "--------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución FIFO)." << endl
        << "--------------------------------------------------------" << endl
        << flush ;

   thread hebra_productora ( funcion_hebra_productora_FIFO ),
          hebra_consumidora( funcion_hebra_consumidora_FIFO);

   hebra_productora.join() ;
   hebra_consumidora.join() ;

   test_contadores();

	 cout << "--------------------------------------------------------" << endl
				<< "Problema de los productores-consumidores (solución LIFO)." << endl
				<< "--------------------------------------------------------" << endl
				<< flush ;

	 hebra_productora= thread( funcion_hebra_productora_LIFO ), hebra_consumidora= thread( funcion_hebra_consumidora_LIFO);

	 hebra_productora.join() ;
	 hebra_consumidora.join() ;

	 test_contadores();
}
