// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: nprodsncons_LIFO.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario
// (versión con n productores y n consumidores)
//
// Historial:
// Creado en diciembre de 2018
// -----------------------------------------------------------------------------

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   nprods= 4,
   ncons= 5,
   etiq_prod= 0,
   etiq_cons= 1,
   etiq_buffer= 2,
   id_buffer             = nprods ,
   num_procesos_esperado = ncons + nprods +1 ,
   num_items             = nprods*ncons*2,
   k_prods= num_items/nprods,
   k_cons= num_items/ncons,
   tam_vector            = 20;

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
// ---------------------------------------------------------------------
// producir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir(int orden) {
   static int contador = orden*k_prods ;
   sleep_for( milliseconds( aleatorio<10,100>()) );
   contador++ ;
   cout << "Productor ha producido valor " << contador << endl << flush;
   return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor(int orden) {
   int valor_prod;
   for ( unsigned int i= 0 ; i < k_prods ; i++ ) {
      // producir valor
      valor_prod = producir(orden);
      // enviar valor
      cout << "Productor va a enviar valor " << valor_prod << endl << flush;
      MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, etiq_prod, MPI_COMM_WORLD ); // enviamos con la etiqueta de los productores= 0
   }
}
// ---------------------------------------------------------------------

void consumir( int valor_cons, int orden ) {
   // espera bloqueada
   sleep_for( milliseconds( aleatorio<110,200>()) );
   cout << "Consumidor " << orden << " ha consumido valor " << valor_cons << endl << flush ;
}
// ---------------------------------------------------------------------

void funcion_consumidor(int orden) {
   int         peticion,
               valor_rec = 1 ;
   MPI_Status  estado ;

   for( unsigned int i=0 ; i < k_cons; i++ ) {
     // La etiqueta de los consumidores es la 1
      MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD);
      MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, etiq_buffer, MPI_COMM_WORLD, &estado );
      cout << "Consumidor " << orden << " ha recibido valor " << valor_rec << endl << flush ;
      consumir( valor_rec, orden );
   }
}
// ---------------------------------------------------------------------

void funcion_buffer() {
   int        buffer[tam_vector],      // buffer con celdas ocupadas y vacías
              valor,                   // valor recibido o enviado
              primera_ocupada     = 0, // índice de primera celda ocupada
              etiq_aceptable ;
   MPI_Status estado ;                 // metadatos del mensaje recibido

   for( unsigned int i=0 ; i < num_items*2 ; i++ ) {
      // 1. determinar si puede enviar solo prod., solo cons, o todos

      if ( primera_ocupada == 0 )               // si buffer vacío
         etiq_aceptable = etiq_prod ;       // $~~~$ solo prod.
      else if ( primera_ocupada == tam_vector ) // si buffer lleno
         etiq_aceptable = etiq_cons ;      // $~~~$ solo cons.
      else                                          // si no vacío ni lleno
         etiq_aceptable = MPI_ANY_TAG ;     // $~~~$ cualquiera

      // 2. recibir un mensaje del emisor o emisores aceptables

      MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_aceptable, MPI_COMM_WORLD, &estado );

      // 3. procesar el mensaje recibido

      switch( estado.MPI_TAG ) { // leer emisor del mensaje en metadatos

         case etiq_prod: // si ha sido el productor: insertar en buffer
            buffer[primera_ocupada] = valor ;
            primera_ocupada ++ ;
            cout << "Buffer ha recibido valor " << valor << endl ;
            break;

         case etiq_cons: // si ha sido el consumidor: extraer y enviarle
            primera_ocupada--;
            valor = buffer[primera_ocupada] ;

            cout << "Buffer va a enviar valor " << valor << endl ;
            MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_buffer, MPI_COMM_WORLD);
            break;
      }
   }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] ) {
   int id_propio, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual ) {
      // ejecutar la operación apropiada a 'id_propio'
      if ( id_propio < nprods )
         funcion_productor(id_propio);
      else if ( id_propio == nprods )
         funcion_buffer();
      else
         funcion_consumidor(id_propio);
   }
   else {
      if ( id_propio == 0 ) { // solo el primero escribe error, indep. del rol
       cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}
