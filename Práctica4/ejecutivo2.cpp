// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 4. Implementación de sistemas de tiempo real.
//
// Archivo: ejecutivo2.cpp
// Implementación del segundo ejemplo de ejecutivo cíclico:
//
//   Datos de las tareas:
//   ------------
//   Ta.  T    C
//   ------------
//   A  500  100
//   B  500   150
//   C  1000   200
//   D  2000   240
//
//  -------------
//
//  Planificación (con Tm == 2000 ms, Ts= 500)
// En el primer ciclo secundario se tardan 100+150+200= 450ms -> 50 ms de espera
// En el segundo ciclo se tardan 100+150= 250 ms -> 250 ms de espera
// En el tercero se tardan 100+150+240= 490ms -> 10 ms de espera
// En el cuarto nuevamente se tardan 450ms
// Mínimo tiempo de espera-> 10 ms.
// Si D tuviese un tiempo de cómputo de 250ms sí sería planificable, podemos acomodar
// lo que sobra en el último ciclo.
//            500       1000      1500      2000
//  *---------*----------*---------*--------*
//  | A B C   | A B     | A B D   | A B C  |
//  *---------*----------*---------*--------*
// Modificación adicional: cada vez que acaba un ciclo secundario se informa del retraso
// del instante final actual respecto al instante final esperado. Si se comprueba
// que dicho retraso es superior a 20 milisegundos, el programa aborta con un mensaje de error.
//
// Historial:
// Creado en Diciembre de 2018
// -----------------------------------------------------------------------------

#include <string>
#include <iostream> // cout, cerr
#include <thread>
#include <chrono>   // utilidades de tiempo
#include <ratio>    // std::ratio_divide

using namespace std ;
using namespace std::chrono ;
using namespace std::this_thread ;

// tipo para duraciones en segundos y milisegundos, en coma flotante:
typedef duration<float,ratio<1,1>>    secondsf ;
typedef duration<float,ratio<1,1000>> millisecondsf ;

// -----------------------------------------------------------------------------
// tarea genérica: duerme durante un intervalo de tiempo (de determinada duración)

void Tarea( const string & nombre, milliseconds tcomputo ) {
   cout << "   Comienza tarea " << nombre << " (C == " << tcomputo.count() << " ms.) ... " ;
   sleep_for( tcomputo );
   cout << "fin." << endl ;
}

// -----------------------------------------------------------------------------
// tareas concretas del problema:

void TareaA() { Tarea( "A", milliseconds(100) );  }
void TareaB() { Tarea( "B", milliseconds( 150) );  }
void TareaC() { Tarea( "C", milliseconds( 200) );  }
void TareaD() { Tarea( "D", milliseconds( 240) );  }


// -----------------------------------------------------------------------------
// implementación del ejecutivo cíclico:

int main( int argc, char *argv[] ) {
   // Ts = duración del ciclo secundario
   const milliseconds Ts( 250 );

   // ini_sec = instante de inicio de la iteración actual del ciclo secundario
   time_point<steady_clock> ini_sec = steady_clock::now(), fin;
   millisecondsf duracion_real;

   while( true ){ // ciclo principal
      cout << endl
           << "---------------------------------------" << endl
           << "Comienza iteración del ciclo principal." << endl ;

      for( int i = 1 ; i <= 4 ; i++ ){ // ciclo secundario (4 iteraciones)
         cout << endl << "Comienza iteración " << i << " del ciclo secundario." << endl ;

         switch( i ) {
            case 1 : TareaA(); TareaB(); TareaC();           break ;
            case 2 : TareaA(); TareaB();                     break ;
            case 3 : TareaA(); TareaB(); TareaD();           break ;
            case 4 : TareaA(); TareaB(); TareaC();           break ;
         }

         ini_sec += Ts ;

         // esperar hasta el inicio de la siguiente iteración del ciclo secundario
         sleep_until( ini_sec );
         fin= steady_clock::now();

         // Hallamos el tiempo transcurrido desde el inicio del ciclo hasta el fin real de las tareas.
         duracion_real= fin - ini_sec;
         cout << "Instante final actual: " << millisecondsf(duracion_real).count() << endl;

      }
   }
}
