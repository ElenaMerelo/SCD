// -----------------------------------------------------------------------------
// Sistemas concurrentes y Distribuidos.
// Seminario 1. Programación Multihebra y Semáforos.
//
// Ejemplo 9 (ejemplo9.cpp)
// Calculo de pi secuencial y concurrentemente
//
//Para compilar desde la terminal: g++ -std=c++11 -pthread ejemplo09-plantilla.cpp -o ejemplo09
// Historial:
// Creado en Abril de 2017
// Modificado en septiembre de 2018 para añadir cálculo concurrente (el otro venía dado).
// -----------------------------------------------------------------------------

#include <iostream>
#include <iomanip>
#include <chrono>  // incluye now, time\_point, duration
#include <future>
#include <vector>
#include <cmath>

using namespace std ;
using namespace std::chrono;

const long m  = 1024l*1024l*1024l, n  = 4  ;

// -----------------------------------------------------------------------------
// evalua la función $f$ a integrar ($f(x)=4/(1+x^2)$)
double f( double x ){ return 4.0/(1.0+x*x); }

// -----------------------------------------------------------------------------
// calcula la integral de forma secuencial, devuelve resultado:
double calcular_integral_secuencial() {
   double suma = 0.0 ;                        // inicializar suma
   for( long i = 0 ; i < m ; i++ )            // para cada $i$ entre $0$ y $m-1$:
      suma += f( (i+double(0.5)) /m );         //   $~$ añadir $f(x_i)$ a la suma actual
   return suma/m ;                            // devolver valor promedio de $f$
}

// -----------------------------------------------------------------------------
/* Función que ejecuta cada hebra: recibe i==índice de la hebra (i<n) y evalúa f
desde el comienzo del intervalo, m/n (lo que viene siendo el tamaño de un chunk,
del intervalo) multiplicado por la hebra que es.Cada hebra calcula un bloque: hebra
1 primer intervalo formado por cuatro subintervalos, hebra 2 por los siguientes cuatro, y así.*/
double funcion_hebra_contigua( long i ) {
  double suma_hebra= 0.0;
  for( long j= m/n*i; j< m/n*(i+1); j++)
    suma_hebra += f((j + double(0.5)) /m);

  return suma_hebra/m;
}

// -----------------------------------------------------------------------------
/* Función que ejecuta cada hebra: recibe i==índice de la hebra (i<n) y evalúa f
desde el comienzo del intervalo, m/n (lo que viene siendo el tamaño de un chunk,
del intervalo) multiplicado por la hebra que es.Cada hebra calcula un bloque: hebra
1 primer subintervalo, hebra 2 el segundo,... hebra 1 el quinto, y así sucesivamente.*/
double funcion_hebra_entrelazada( long i ) {
  double suma_hebra= 0.0;
  for( long j= i; j< m ; j += n)
    suma_hebra += f((j + double(0.5)) /m);;

  return suma_hebra/m;
}

// -----------------------------------------------------------------------------
/* Cálculo de la integral de forma concurrente. Una vez las hebras han calculado
las sumas parciales de los valores de f, la hebra principal las recoge en una suma total,
repartiendo los bloques entre las hebras de manera entrelazada.*/
double calcular_integral_concurrente_entrelazada( ) {
  double suma= 0.0;
  future<double> futuros[n];

  //Ponemos en marcha todas las hebras y obtenemos los futuros
  for( long i= 0; i< n; i++)
    futuros[i] = async( launch::async, funcion_hebra_entrelazada, i);
  //Esperamos a que cada hebra termine y vamos obteneniendo el resultado de la integral
  for(long i= 0; i<n; i++)
    suma += futuros[i].get();

  return suma;
}

// -----------------------------------------------------------------------------
/* Cálculo de la integral de forma concurrente. Una vez las hebras han calculado
las sumas parciales de los valores de f, la hebra principal las recoge en una suma total,
repartiendo los bloques entre las hebras de manera contigua.*/
double calcular_integral_concurrente_contigua( ) {
  double suma= 0.0;
  future<double> futuros[n];

  //Ponemos en marcha todas las hebras y obtenemos los futuros
  for( long i= 0; i< n; i++)
    futuros[i] = async( launch::async, funcion_hebra_contigua, i);
  //Esperamos a que cada hebra termine y vamos obteneniendo el resultado de la integral
  for(long i= 0; i<n; i++)
    suma += futuros[i].get();

  return suma;
}
// -----------------------------------------------------------------------------

int main() {
  time_point<steady_clock> inicio_sec  = steady_clock::now() ;
  const double             result_sec  = calcular_integral_secuencial(  );
  time_point<steady_clock> fin_sec     = steady_clock::now() ;

  double x = sin(0.4567);
  time_point<steady_clock> inicio_conc_ent = steady_clock::now() ;
  const double             result_conc_ent = calcular_integral_concurrente_entrelazada(  );
  time_point<steady_clock> fin_conc_ent    = steady_clock::now() ;

  time_point<steady_clock> inicio_conc_cont = steady_clock::now() ;
  const double             result_conc_cont = calcular_integral_concurrente_contigua(  );
  time_point<steady_clock> fin_conc_cont   = steady_clock::now() ;

  duration<float,milli>    tiempo_sec  = fin_sec  - inicio_sec ,
                           tiempo_conc_ent = fin_conc_ent - inicio_conc_ent,
                           tiempo_conc_cont= fin_conc_cont - inicio_conc_cont;
  const float              porc_ent        = 100.0*tiempo_conc_ent.count()/tiempo_sec.count(),
                           porc_cont       = 100.0*tiempo_conc_cont.count()/tiempo_sec.count();


  constexpr double pi = 3.14159265358979323846l ;

  cout << "Número de muestras (m)   : " << m << endl
       << "Número de hebras (n)     : " << n << endl
       << setprecision(18)
       << "Valor de PI              : " << pi << endl
       << "Resultado secuencial     : " << result_sec  << endl
       << "Resultado concurrente entrelazada   : " << result_conc_ent << endl
       << "Resultado concurrente contigua   : " << result_conc_cont << endl
       << setprecision(5)
       << "Tiempo secuencial        : " << tiempo_sec.count()  << " milisegundos. " << endl
       << "Tiempo concurrente entrelazada      : " << tiempo_conc_ent.count() << " milisegundos. " << endl
       << "Tiempo concurrente contigua      : " << tiempo_conc_cont.count() << " milisegundos. " << endl
       << setprecision(4)
       << "Porcentaje t.conc_entrelazada/t.sec. : " << porc_ent << "%" << endl
       << "Porcentaje t.conc_contigua/t.sec. : " << porc_cont << "%" << endl;
}
