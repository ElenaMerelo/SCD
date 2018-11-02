// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// Ejemplo de un monitor en C++11 con semántica SU, para el problema
// del productor/consumidor, con n productores y n consumidores.
// Versión acorde a la explicación de cómo hacerlo en las diapositivas
// Opcion LIFO (stack)
//
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <mutex>
#include "HoareMonitor.h"
#include <random>

using namespace std ;
using namespace HM;

constexpr int num_items  = 40 ;     // número de items a producir/consumir
const int nprods= 2, ncons= 5; //número de productores y consumidores, ha de ser múltiplo del número de ítems

mutex mtx ;                 // mutex de escritura en pantalla
unsigned cont_prod[num_items], // contadores de verificación: producidos
         cont_cons[num_items]; // contadores de verificación: consumidos

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

int producir_dato(int i) {
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "Productor: " << i << ", producido: " << contador << endl << flush ;
   mtx.unlock();
   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato, int num_hebra ){
   if ( num_items <= dato ){
      cout << "Consumidor: " << num_hebra << ", dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores(){
   for( unsigned i = 0 ; i < num_items ; i++ ){
     cont_prod[i] = 0 ;
     cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores() {
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SU, n prod. y n cons.

class ProdConsNSC: public HoareMonitor{
 private:
 static const int           // constantes:
   num_celdas_total = 10;   //  núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
   primera_libre ;          //  indice de celda de la próxima inserción
 CondVar        // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdConsNSC(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdConsNSC::ProdConsNSC(  ){
   primera_libre = 0 ;
   ocupadas= newCondVar();
   libres= newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsNSC::leer(){
   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   while ( primera_libre == 0 )
      ocupadas.wait();

   // hacer la operación de lectura, actualizando estado del monitor
   assert( 0 < primera_libre  );
   primera_libre-- ;

   const int valor = buffer[primera_libre] ;

   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdConsNSC::escribir( int valor ){
   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   while ( primera_libre == num_celdas_total )
      libres.wait();

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( primera_libre < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   primera_libre++ ;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();

}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdConsNSC> monitor, int num_items_prod, int num_hebra ){
  int valor;
   for( unsigned i = 0 ; i < num_items_prod ; i++ ){
      valor = producir_dato(num_hebra) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdConsNSC> monitor, int num_items_cons, int num_hebra ){
    int valor;
   for( unsigned i = 0 ; i < num_items_cons ; i++ ){
      valor = monitor->leer();
      consumir_dato( valor, num_hebra ) ;
   }
}
// -----------------------------------------------------------------------------

int main(){
  int i;
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (n prod/cons, Monitor SU, buffer LIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdConsNSC> monitor= Create<ProdConsNSC>() ;

   thread productores[nprods], consumidores[ncons];

   for(i= 0; i< nprods; i++)
     productores[i]= thread(funcion_hebra_productora, monitor, num_items/nprods, i);

   for(i= 0; i< ncons; i++)
     consumidores[i]= thread(funcion_hebra_consumidora, monitor, num_items/ncons, i);

   for(i= 0; i< nprods; i++)
     productores[i].join() ;

   for(i= 0; i< ncons; i++)
     consumidores[i].join() ;

   test_contadores();
}
