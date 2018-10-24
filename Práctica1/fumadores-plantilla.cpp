#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//Variables globales
const int num_fumadores= 4;
Semaphore disponible_ingrediente[num_fumadores]= {0, 0, 0, 0};

Semaphore mostrador_vacio(1); //vale 1 si el mostrador está libre, 0 si ocupado. Inicialmente no hay nada sobre él

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------
template< int min, int max > int aleatorio(){
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//----------------------------------------------------------------------
//Función usada por el estanquero para producir un número aleatorio entre 0 y 2 inclusives, con un cierto retraso aleatorio
int producir(){
  this_thread::sleep_for( chrono::milliseconds( aleatorio<20, 200>() ) );
  return aleatorio<0, num_fumadores-1>();
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero
void funcion_hebra_estanquero( ){
  int i, tope= 20;
  //for(int j= 0; j< tope; j++){ //descomentar si queremos que pare de producir
  while(true){ //descomentar y comentar línea superior para que se produzcan ingredientes durante un tiempo indefinido
    i= producir();
    sem_wait(mostrador_vacio); //esperamos a que el mostrador esté vacío para producir un nuevo ingrediente
    cout << "\nEstanquero produce ingrediente " << i; //sentecia Pi
    sem_signal(disponible_ingrediente[i]); //se señala que s[i] puede consumir
  }
  exit(0);
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra
void fumar( int num_fumador ){
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
   cout << "\nFumador " << num_fumador << "  :"
        << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
   cout << "\nFumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente " << num_fumador << endl;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador número i
void  funcion_hebra_fumador( int i ){
   while( true ){
     sem_wait(disponible_ingrediente[i]);
     cout << "\nEl fumador " << i << " retira su ingrediente";
     sem_signal(mostrador_vacio);
     fumar(i);
   }
}

//----------------------------------------------------------------------

int main(){
  unsigned i;
  thread fumadores[num_fumadores], estanquero(funcion_hebra_estanquero);

  for(i= 0; i< num_fumadores; i++)
    fumadores[i]= thread(funcion_hebra_fumador, i);

  estanquero.join();

  for(i= 0; i< num_fumadores; i++)
    fumadores[i].join();
}
