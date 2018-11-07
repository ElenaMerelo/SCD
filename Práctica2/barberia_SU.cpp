#include <iostream>
#include <iomanip>
#include <random>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;


class  Barberia: public HoareMonitor {
private:
  static const int n= 15;
  CondVar sala_espera; //cola condición para los que esperan en la sala de espera
  CondVar silla_pelar; //cola de condición que indica si hay alguien en la silla de pelar
  bool barbero_durmiendo;
public:
  Barberia();
  void cortarPelo(int i); // llamado por el cliente i para obtener servicio del barbero, despertándolo o esperando a que termine con el cliente anterior
  void siguienteCliente(); //llamada por el barbero para esperar la llegada de un nuevo cliente y servirlo
  void finCliente(); // llamada por el barbero cuando termina de pelar al cliente actual

};

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
Barberia::Barberia(){
  sala_espera= newCondVar();
  silla_pelar= newCondVar();

  //Suponemos que inicialmente no hay nadie en la barbería, por lo que el barbero duerme
  barbero_durmiendo= true;
}

//----------------------------------------------------------------------
//Espera bloqueada de duración aleatoria
void esperarFueraBarberia(int i){
  chrono::milliseconds duracion_espera( aleatorio<20, 200> () );

  cout << "\nCliente " << i << " espera fuera de la barbería "
        << duracion_espera.count() << " milisegundos" << endl;
  this_thread::sleep_for( duracion_espera );
}

//----------------------------------------------------------------------
void Barberia::cortarPelo(int i){
  if(!barbero_durmiendo){ //si el barbero está ocupado pelando
    cout << "\nEl barbero está ocupado: cliente " << i << " va a sala de espera\n";
    sala_espera.wait(); //mandamos al cliente nuevo a la sala de espera
  }

  //Si no
  barbero_durmiendo= false; //despertamos al barbero
  sala_espera.signal(); //sacamos al cliente que lleva más tiempo esperando de la sala de espera
  silla_pelar.signal(); //lo sentamos en la sala de espera

}

//----------------------------------------------------------------------
void funcion_hebra_cliente(MRef<Barberia> monitor, int i){
  while(true){
    monitor->cortarPelo(i);
    esperarFueraBarberia(i);
  }
}

//----------------------------------------------------------------------
//Cuando el barbero sale de siguiente cliente éste ya debe de estar sentado en la silla de pelar
void Barberia::siguienteCliente(){
  if(sala_espera.empty() && silla_pelar.empty()){
    cout << "\nNo hay ningún cliente: barbero duerme";
    barbero_durmiendo= true; //si no hay ningún cliente que pelar se echa a dormir el barbero
    silla_pelar.wait();
  }

  //Si no hay cliente en la silla de pelar, lo llamamos y sentamos
  if(silla_pelar.empty()){
    cout << "\nNo hay cliente sentado: que pase uno de la sala de espera.\n";
    if(! sala_espera.empty() )
      sala_espera.signal();
  }

  silla_pelar.signal();
  barbero_durmiendo= false; //despertamos al barbero, ya hay un cliente que puede pelar
}

//----------------------------------------------------------------------
//Espera bloqueada de duración aleatoria
void cortarPeloACliente(){
  chrono::milliseconds duracion_corte( aleatorio<20, 200> () );

  cout << "\nDuración corte pelo: " << duracion_corte.count() << " milisegundos." << endl;
  this_thread::sleep_for( duracion_corte );
}

//----------------------------------------------------------------------
void Barberia::finCliente(){
  cout << "\nFin de cliente\n";
}


//----------------------------------------------------------------------
void funcion_hebra_barbero(MRef<Barberia> monitor){
  while(true){
    monitor->siguienteCliente();
    cortarPeloACliente();
    monitor->finCliente();
  }
}

int main(){
  MRef<Barberia> monitor= Create<Barberia>();
  int num_clientes= 10, i;
  thread clientes[num_clientes], barbero(funcion_hebra_barbero, monitor);

  for(i= 0; i< num_clientes; i++)
    clientes[i]= thread(funcion_hebra_cliente, monitor, i );

  barbero.join();
  for(i= 0; i< num_clientes; i++)
    clientes[i].join();
}








//
