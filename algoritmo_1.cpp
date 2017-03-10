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
    int id_video;
};

bool operator< (const conection& lhs, const conection& rhs){
    return lhs.id_endpoint*1000000+lhs.id_cache+1000+lhs.id_video <  rhs.id_endpoint*1000000+rhs.id_cache+1000+rhs.id_video;
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
//key->saved_time*requests value->connections video
map<int,set<conection>> latencias_ordenadas;

//videos index->num value valor
int size_of_videos[MAX_SIZE];

//endpoints
//key->endpoint value->connected caches
map<int,set<int>> endpoints_cache;

//requests
//Index-> Endpoint First->video Second->Num_Requests
map<int,int> numrequ_endp_videos[MAX_SIZE];

//Registro de los caches
cache cache_register[MAX_SIZE];

//Variables error
bool error_cargar=false;
bool error_ejecuacion=false;
bool error_guardar=false;

void cargarRequests(){
    ifstream f;
    int num_requests;
    int size_caches;
    char rubish[100];

    f.open(nombre_fichero_entrada);

    if (f.is_open()) {
        cout << "Fichero de entrada abierto" << endl;

        f >> num_videos >> num_endpoints >> num_requests >> num_caches >> size_caches;
        f.getline(rubish, 1);
        int rubish_int;

        for (int i = 0; i < num_videos; ++i) {//cargar videos
            f >> rubish_int;
        }
        f.getline(rubish, 1);

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
            }

        }

        for (int i = 0; i < num_requests; ++i) {
            int vid;
            int end;
            int req;
            f >> vid >> end >> req;

            (numrequ_endp_videos[end])[vid]=req;
            f.getline(rubish, 1);
        }

        cout << "Requests cargados" << endl;
    }else{
        cout << "Error: Fichero de entrada no abierto" <<endl;
        error_cargar= true;

    }
    f.close();

}

/*
* Carga los datos en las estructuras correspondientes
*/
void cargarDatos() {
    ifstream f;
    int num_requests;
    int size_caches;
    char rubish[100];
    map<int,set<conection>> latemcias_edpoint_provisional;

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
        }

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

                lat=lat-datacenter_latency; //Mayores latencias posiciones mas bajas

                if(lat >= 0){//No añade el cache
                    continue;
                }
                endpoints_cache[i].insert(cach);

                conection insertar;
                insertar.id_endpoint=i;
                insertar.id_cache=cach;

                //Inseta todos los videos
                map<int,int>::const_iterator iter_numrequ_endp_videos = numrequ_endp_videos[insertar.id_endpoint].begin();

                while(iter_numrequ_endp_videos!=numrequ_endp_videos[insertar.id_endpoint].end()){
                    insertar.id_video=iter_numrequ_endp_videos->first;
                    latencias_ordenadas[lat*(iter_numrequ_endp_videos->second)].insert(insertar);
                    ++iter_numrequ_endp_videos;
                }
            }

        }

        cout << "Datos cargados" << endl;
    }else{
        cout << "Error: Fichero de entrada no abierto" <<endl;
        error_cargar= true;

    }
    f.close();
}

/*
* Algoritmo de distribución de los videos en los caches correspondientes
*/
void distribuirVideos(){
    cout << "Inicio distribucion videos" << endl;

    map<int,set<conection>>::const_iterator iter_latencias_ordenadas = latencias_ordenadas.begin();

    int num_endpoints_actual_processed=0;
    int num_max_endpoints_to_process=latencias_ordenadas.size();

    //Realizo primera vuelta
    while (iter_latencias_ordenadas != latencias_ordenadas.end()){//recorro todas las latencias
        cout << num_endpoints_actual_processed << "/" << 2*num_max_endpoints_to_process <<endl;
        ++num_endpoints_actual_processed;

        set<conection>::const_iterator iter_latencias_ordenadas_subset = iter_latencias_ordenadas->second.begin();

        while (iter_latencias_ordenadas_subset != iter_latencias_ordenadas->second.end()) {//recorro todas las latencias del subgrupo
            bool video_almacenado = false;
            for (int j : endpoints_cache.find(iter_latencias_ordenadas_subset->id_endpoint)->second) {

                if(cache_register[j].videos.find(iter_latencias_ordenadas_subset->id_video)!=cache_register[j].videos.end())
                {
                    video_almacenado=true;
                    break;
                }
            }

            if(!video_almacenado){
                if (cache_register[iter_latencias_ordenadas_subset->id_cache].free_size>=size_of_videos[iter_latencias_ordenadas_subset->id_video]){
                    cache_register[iter_latencias_ordenadas_subset->id_cache].free_size-= size_of_videos[iter_latencias_ordenadas_subset->id_video];
                    cache_register[iter_latencias_ordenadas_subset->id_cache].videos.insert(iter_latencias_ordenadas_subset->id_video);
                }
            }

            ++iter_latencias_ordenadas_subset;
        }
        ++iter_latencias_ordenadas;
    }

    //Realizo segunda vuelta
    while (iter_latencias_ordenadas != latencias_ordenadas.end()){//recorro todas las latencias
        cout << num_endpoints_actual_processed << "/" << 2*num_max_endpoints_to_process <<endl;
        ++num_endpoints_actual_processed;

        set<conection>::const_iterator iter_latencias_ordenadas_subset = iter_latencias_ordenadas->second.begin();

        while (iter_latencias_ordenadas_subset != iter_latencias_ordenadas->second.end()) {//recorro todas las latencias del subgrupo
            bool video_almacenado=cache_register[iter_latencias_ordenadas_subset->id_cache].videos.find(iter_latencias_ordenadas_subset->id_video)!=cache_register[iter_latencias_ordenadas_subset->id_cache].videos.end();

            if(!video_almacenado){
                if (cache_register[iter_latencias_ordenadas_subset->id_cache].free_size>=size_of_videos[iter_latencias_ordenadas_subset->id_video]){
                    cache_register[iter_latencias_ordenadas_subset->id_cache].free_size-= size_of_videos[iter_latencias_ordenadas_subset->id_video];
                    cache_register[iter_latencias_ordenadas_subset->id_cache].videos.insert(iter_latencias_ordenadas_subset->id_video);
                }
            }

            ++iter_latencias_ordenadas_subset;
        }
        ++iter_latencias_ordenadas;
    }


    cout << "Videos distribuidos" << endl;
}

/*
* Guarda el contenido de cache_register en los caches correspondientes
*/
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


    cargarRequests();

    cargarDatos();
    if(error_cargar) return -1;

    distribuirVideos();

    if(error_ejecuacion) return -1;

    salida();

    return 0;
}
