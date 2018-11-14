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
void espera(int i){
  chrono::milliseconds duracion_espera( aleatorio<20, 200> () );
  cout << "\nHebra " << i << " espera " << duracion_espera.count() << " milisegundos" << endl;
  this_thread::sleep_for( duracion_espera );
}

//----------------------------------------------------------------------
class recurso: public HoareMonitor {
private:
  static const int num_items= 2;
  CondVar tipo[num_items];
  int libres_tipo[num_items];

public:
  recurso(){
    for(int i= 0; i< num_items; i++){
      tipo[i]= newCondVar();
      tipo[i]= newCondVar();
      libres_tipo[i]= 2;
    }
  }

  void acceder(int thread){
    int tr= (thread % 2 == 0)? 1: 2;
      if(libres_tipo[tr-1] == 0){ // si no hay libres esperamos
        cout << "\nHebra " << thread << " espera a que haya recursos de tipo " << tr << " disponibles.";
        tipo[tr-1].wait();
      }
      else {
        tipo[tr-1].signal();
        libres_tipo[tr-1]--;
        cout << "\nHebra " << thread << " consume recurso de tipo " << tr << ". Quedan " << libres_tipo[tr-1] << " recursos de tipo " << tr;
      }
    }

    void fin_acceso(int thread){
      int tr= (thread % 2 == 0)?  1: 2;
      libres_tipo[tr-1]++;
      tipo[tr-1].signal();
      cout << "\nHebra " << thread << " libera recurso. Ahora hay " << libres_tipo[tr-1] << " recursos disponibles de tipo " << tr;
    }
};

void funcion_hebra(MRef<recurso> monitor, int i){
  while(true){
    monitor->acceder(i);
    espera(i);
    monitor->fin_acceso(i);
    espera(i);
  }
}

int main(){
  MRef<recurso> monitor= Create<recurso>();
  int n_hebras= 8, i;

  thread hebras[n_hebras];

  for(i= 0; i< n_hebras; i++)
    hebras[i]= thread(funcion_hebra, monitor, i);

  for(i= 0; i< n_hebras; i++)
    hebras[i].join();
}
