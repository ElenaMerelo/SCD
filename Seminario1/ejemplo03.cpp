// -----------------------------------------------------------------------------
// Sistemas concurrentes y Distribuidos.
// Seminario 1. Programación Multihebra y Semáforos.
//
// Ejemplo 3 (ejemplo3.cpp)
// Obtención de resultados mediante variables globales
//
// Historial:
// Creado en Abril de 2017
// Modificado en septiembre de 2018 para hacerlo con un vector global 
// -----------------------------------------------------------------------------

#include <iostream>
#include <future>     // declaracion de {\bf std::thread}, {\bf std::async}, {\bf std::future}
using namespace std ; // permite acortar la notación (abc en lugar de std::abc)

// declaración de la función {\bf factorial} (parámetro {\bf int}, resultado {\bf long})
long factorial( int n ) { return n > 0 ? n*factorial(n-1) : 1 ; }

// variables globales donde se escriben los resultados
long resultado1, resultado2 ;
long resultados[2];

// funciones que ejecutan las hebras
void funcion_hebra_1( int n ) { resultado1 = factorial( n ) ; }
void funcion_hebra_2( int n ) { resultado2 = factorial( n ) ; }

void funcion_hebras(int n, int id) { resultados[id]= factorial (n); }

int main(){
  // iniciar las hebras
  thread hebra1( funcion_hebra_1, 5 ), // calcula factorial(5) en resultado1
         hebra2( funcion_hebra_2, 10 ); // calcula factorial(10) en resultado1
  thread hebra3(funcion_hebras, 5, 0),
         hebra4(funcion_hebras, 10, 1);

  // esperar a que terminen las hebras,
  hebra1.join();
  hebra2.join();

  hebra3.join();
  hebra4.join();

  // imprimir los resultads:
  cout << "factorial(5)  == " << resultado1 << endl;
  cout << "factorial(10) == " << resultado2 << endl;

  cout << "factorial(5)  == " << resultados[0] << endl;
  cout << "factorial(10) == " << resultados[1] << endl;
}
