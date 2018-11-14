#include <iostream>
#include <iomanip>
#include <random>
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;

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
int producirIngrediente(){
  this_thread::sleep_for( chrono::milliseconds( aleatorio<20, 200>() ) );
  return aleatorio<0, 2>();
}

static int veces_ha_fumado= 0;

//----------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra
void fumar( int num_fumador ){
    // calcular milisegundos aleatorios de duración de la acción de fumar)
    chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

    if(num_fumador == 2 && (veces_ha_fumado % 4 == 0))
      cout << "\nFumador 2 ha fumado " << veces_ha_fumado << " veces. Decide no fumar.\n";
    else{
      // informa de que comienza a fumar
      cout << "\nFumador " << num_fumador << "  :"
      << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

      if(num_fumador == 2)
        veces_ha_fumado++;
    }
    // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
    this_thread::sleep_for( duracion_fumar );

    // informa de que ha terminado de fumar
    cout << "\nFumador " << num_fumador << "  : termina , comienza espera de ingrediente " << num_fumador << endl;

}

class Estanco: public HoareMonitor {
private:
  int ingrediente; //si ingrediente= -1 no hay ningún ingrediente en el mostrador
  CondVar fumador[3]; //colas de condición distintas para cada uno de los fumadores, al esperar cada uno una cosa
  CondVar estanquero; //cola formada por los ingredientes producidos por el estanquero que aún no han sido consumidos
  CondVar fumador2; // nueva cola condición para avisar al fumador 2 cuando no quiera fumar
public:
  Estanco();
  void obtenerIngrediente(int i);
  void ponerIngrediente(int i);
  void esperarRecogidaIngrediente();
  int producirIngrediente();
};


//----------------------------------------------------------------------
Estanco::Estanco(){
  ingrediente= -1; //todavía no se ha producido ningún ingrediente
  fumador2= newCondVar();
  for(int i= 0; i< 3; i++)
    fumador[i]= newCondVar();

  estanquero= newCondVar();
}


//----------------------------------------------------------------------
void Estanco::ponerIngrediente(int i){
  cout << "\nEstanquero pone ingrediente " << i; //sentecia Pi
  ingrediente= i;
  fumador[i].signal();

  fumador2.wait(); // espera

  if(veces_ha_fumado % 4 == 0 && i == 2)
   fumador2.signal(); // señalamos cuando el fumador 2 haya fumado tres veces

}

//----------------------------------------------------------------------
void Estanco::esperarRecogidaIngrediente(){
  if(ingrediente== -1)
    estanquero.wait();

}

//----------------------------------------------------------------------
void Estanco::obtenerIngrediente(int i){
  for(int j= 0; j< 3, j=! i; j++)
    fumador[j].wait();

  if(i =! 2 || veces_ha_fumado % 4 != 0)
    fumador2.wait();


  cout << "\nEl fumador " << i << " retira su ingrediente";

  ingrediente= -1; //vaciamos el mostrador
  estanquero.signal();
}

//----------------------------------------------------------------------
void funcion_hebra_estanquero(MRef<Estanco> monitor){
  int i;
  while(true){
    i= producirIngrediente();
    monitor->ponerIngrediente(i);
    monitor->esperarRecogidaIngrediente();
  }
}

//----------------------------------------------------------------------
void funcion_hebra_fumador(MRef<Estanco> monitor, int i){
  //veces_ha_fumado= 0;
  while(true){
    monitor->obtenerIngrediente(i);
    fumar(i);
  }
}

//----------------------------------------------------------------------
int main(){
  MRef<Estanco> monitor= Create<Estanco>();
  int num_fumadores= 3, i;
  thread fumadores[num_fumadores], estanquero(funcion_hebra_estanquero, monitor);


  for(i= 0; i< num_fumadores; i++)
    fumadores[i]= thread(funcion_hebra_fumador, monitor, i);

  estanquero.join();

  for(i= 0; i< num_fumadores; i++)
    fumadores[i].join();
}
