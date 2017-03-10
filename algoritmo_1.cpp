
//
// Utilización:
// Primer parámetro = ruta fichero entrada
// Segundo parámetro = ruta fichero salida
//

#include <iostream>
#include <list>
#include <set>
#include <fstream>
#include <map>

using namespace std;
const int MAX_SIZE=100000;

struct conection{
    int id_endpoint;
    int id_cache;


};

bool operator< (const conection& lhs, const conection& rhs){
    return lhs.id_endpoint*10000+lhs.id_cache <  rhs.id_endpoint*10000+rhs.id_cache;
}

struct cache{
    int free_size;
    set<int> videos;
};

//Nombres ficheros
char *nombre_fichero_entrada;
char *nombre_fichero_salida;

//Numeros de datos
int num_videos;
int num_caches;
int num_endpoints;

//latencias ordenadas
//key->saved_time value->connections
map<int,set<conection>> latencias_ordenadas;

//videos index->num value valor
int size_of_videos[MAX_SIZE];

//endpoints
//key->endpoint value->connected caches
map<int,set<int>> endpoints_cache;

//endpoints_videos
//key->endpoint value->videos visto por el endpoint en cuestion
map<int,set<int>> endpoints_videos;

//Registro de los caches
cache cache_register[MAX_SIZE];

//Variables error
bool error_cargar=false;
bool error_ejecuacion=false;
bool error_guardar=false;

/*
* Carga los datos en las estructuras correspondientes
*/
void cargarDatos() {
    ifstream f;
    int num_requests;
    int size_caches;
    char rubish[100];

    f.open(nombre_fichero_entrada);

    if (f.is_open()) {
        cout << "Fichero de entrada abierto" << endl;

        f >> num_videos >> num_endpoints >> num_requests >> num_caches >> size_caches;
        f.getline(rubish, 1);

        for (int i = 0; i < num_videos; ++i) {//cargar videos
            f >> size_of_videos[i];
        }
        f.getline(rubish, 1);

        for (int i=0; i < num_caches; ++i) {// Seteo caches
            cache_register[i].free_size=size_caches;
        }//Hasta aqui va bien


        for (int i = 0; i < num_endpoints; ++i) {
            int number_of_connected_caches;
            int datacenter_latency;

            f >> datacenter_latency;
            f >> number_of_connected_caches;
            f.getline(rubish, 1);

            for (int j = 0; j < number_of_connected_caches; ++j) {

                int lat;
                int cach;

                f >> cach;
                f >> lat;

                f.getline(rubish, 1);


                lat=datacenter_latency-lat;

                if(lat <= 0){//ese cache no tiene sentido anyadirlo
                    continue;
                }
                endpoints_cache[i].insert(cach);

                lat=-lat;//Mayores latencias posiciones mas bajas

                conection insertar;
                insertar.id_endpoint=i;
                insertar.id_cache=cach;
                //set_de_conexiones_actual->insert(insertar);
                latencias_ordenadas[lat].insert(insertar);

            }
        }

        for (int i = 0; i < num_requests; ++i) {
            int vid;
            int end;
            int req;
            f >> vid >> end >> req;

            endpoints_videos[end].insert(vid);
            f.getline(rubish, 1);
        }

        cout << "Datos cargados" << endl;
    }else{
        cout << "Error: Fichero de entrada no abierto" <<endl;
        error_cargar= true;

    }
    f.close();
}

/*
* Guarda el contenido de cache_register en los caches correspondientes
*/
void distribuirVideos(){
    cout << "Inicio distribucion videos" << endl;

    map<int,set<conection>>::const_iterator iter_latencias_ordenadas = latencias_ordenadas.begin();

    int num_endpoints_actual_processed=0;
    int num_max_endpoints_to_process=latencias_ordenadas.size();

    while (iter_latencias_ordenadas != latencias_ordenadas.end()){//recorro todas las latencias

      cout << num_endpoints_actual_processed << "/" << num_max_endpoints_to_process <<endl;
      ++num_endpoints_actual_processed;

        if(iter_latencias_ordenadas->first>=0) {
            ++iter_latencias_ordenadas;
            continue;
        }

        set<conection>::const_iterator iter_latencias_ordenadas_subset = iter_latencias_ordenadas->second.begin();

        cout<<iter_latencias_ordenadas->second.size()<<endl;

        while (iter_latencias_ordenadas_subset != iter_latencias_ordenadas->second.end()) {//recorro todas las latencias del subgrupo

            conection conexion_actual=*iter_latencias_ordenadas_subset;

            set<int> &videos_conexion_actual = endpoints_videos.find(conexion_actual.id_endpoint)->second;

            set<int> &caches_conexion_actual = endpoints_cache.find(conexion_actual.id_endpoint)->second;

            for (int i : videos_conexion_actual) {
                bool video_almacenado = false;
                for (int j : caches_conexion_actual) {
                    if(cache_register[j].videos.find(i)!=cache_register[j].videos.end())
                    {
                        video_almacenado=true;
                        break;
                    }
                }

                if(!video_almacenado){
                    if (cache_register[conexion_actual.id_cache].free_size>=size_of_videos[i]){
                        cache_register[conexion_actual.id_cache].free_size=cache_register[conexion_actual.id_cache].free_size -  size_of_videos[i];
                        cache_register[conexion_actual.id_cache].videos.insert(i);
                    }
                }
            }
            ++iter_latencias_ordenadas_subset;
        }
        ++iter_latencias_ordenadas;
    }

    cout << "Videos distribuidos" << endl;
}

void salida() {
    ofstream f;
    f.open(nombre_fichero_salida);
    if (f.is_open()) {
        cout << "Fichero salida creado" << endl;
        int num_caches_usados=0;
        for (int j = 0; j < num_caches ; ++j) {
            if(cache_register[j].videos.size()>0) ++num_caches_usados;
        }
        f << num_caches_usados << endl;

        for (int j = 0; j < num_caches ; ++j) {//cargar video
            if(cache_register[j].videos.size()>0){
                f << j ;

                for (set<int>::iterator i = cache_register[j].videos.begin(); i != cache_register[j].videos.end(); ++i) {
                    f << " " << *i ;
                }
                f << endl;
            }
        }
    }else{
        cout << "Error: Al crear el fichero de salida" << endl;
        error_guardar=true;
    }
    f.close();
}

int main(int argc, char *argv[]) {
    if(argc!=3) cout << "Numero de argumentos incorrecto"<<endl;

    nombre_fichero_entrada = argv[1];

    nombre_fichero_salida = argv[2];

    cargarDatos();
    if(error_cargar) return -1;

    distribuirVideos();

    if(error_ejecuacion) return -1;

    salida();

    return 0;
}
