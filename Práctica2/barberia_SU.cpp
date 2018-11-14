#include <iostream>
#include <iomanip>
#include <random>
#include <mutex>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

mutex mtx;

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
void espera(){
  chrono::milliseconds duracion_espera( aleatorio<20, 200> () );
  cout << duracion_espera.count() << " milisegundos" << endl;
  this_thread::sleep_for( duracion_espera );

}

//----------------------------------------------------------------------
//Espera bloqueada de duración aleatoria
void esperarFueraBarberia(int i){
  mtx.lock();
  cout << "\nCliente " << i << " va a esperar fuera de la barbería ";
  espera();
  mtx.unlock();
}

//----------------------------------------------------------------------
//Espera bloqueada de duración aleatoria
void cortarPeloACliente(){
  mtx.lock();
  cout << "\nEl corte de pelo va a durar ";
  espera();
  mtx.unlock();
}


class  Barberia: public HoareMonitor {
private:
  static const int n= 10;
  CondVar sala_espera; //cola condición para los que esperan en la sala de espera
  CondVar barbero; //cola de condición que indica si hay alguien en la silla de pelar
  CondVar silla_pelar;
  bool silla_vacia;
public:
  Barberia();
  void cortarPelo(int i); // llamado por el cliente i para obtener servicio del barbero, despertándolo o esperando a que termine con el cliente anterior
  void siguienteCliente(); //llamada por el barbero para esperar la llegada de un nuevo cliente y servirlo
  void finCliente(); // llamada por el barbero cuando termina de pelar al cliente actual

};


//----------------------------------------------------------------------
Barberia::Barberia(){
  sala_espera= newCondVar();
  barbero= newCondVar();
  silla_pelar= newCondVar();

  //Suponemos que inicialmente no hay nadie en la barbería, por lo que el barbero duerme
  silla_vacia= true;
}


//----------------------------------------------------------------------
void Barberia::cortarPelo(int i){
  cout << "\nCliente " << i << " entra a la barberia";
  if(!sala_espera.empty() || !silla_vacia){ //si hay clientes esperando o la silla está llena
    cout << "\nEl barbero está ocupado: cliente ( " << i << " ) va a sala de espera\n";
    sala_espera.wait(); //mandamos al cliente nuevo a la sala de espera
  }

  //Si no
  barbero.signal(); //despertamos al barbero
  silla_vacia= false; //sentamos al cliente
  cout << "Cliente ( " << i << " ) se sienta." << endl << flush;
  silla_pelar.wait();
  cout << "Cliente ( " << i << " ) se levanta." << endl << flush;}


//----------------------------------------------------------------------
//Cuando el barbero sale de siguiente cliente éste ya debe de estar sentado en la silla de pelar
void Barberia::siguienteCliente(){
  if(sala_espera.empty() && silla_vacia){
    cout << "\nNo hay ningún cliente: barbero duerme";
    barbero.wait(); //si no hay ningún cliente que pelar se echa a dormir el barbero
  }

  else sala_espera.signal();

}


//----------------------------------------------------------------------
void Barberia::finCliente(){
  cout << "\nFin de cliente\n";
  silla_vacia= true;
  silla_pelar.signal();
}

//----------------------------------------------------------------------
void funcion_hebra_barbero(MRef<Barberia> monitor){
  while(true){
    monitor->siguienteCliente();
    cortarPeloACliente();
    monitor->finCliente();
  }
}

//----------------------------------------------------------------------
void funcion_hebra_cliente(MRef<Barberia> monitor, int i){
  while(true){
    monitor->cortarPelo(i);
    esperarFueraBarberia(i);
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
