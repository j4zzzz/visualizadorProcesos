#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <random>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <unistd.h> // Para sysconf(_SC_CLK_TCK)
#include <iterator> // Para std::istream_iterator
#include <chrono>

// Clase Proceso
class Proceso {
public:
    std::string nombre;
    int id;
    int rafaga;
    int tiempoLlegada;
    char estado;

    Proceso(const std::string& nombre, int id, int rafaga, int tiempoLlegada, char estado = 'N')
        : nombre(nombre), id(id), rafaga(rafaga), tiempoLlegada(tiempoLlegada), estado(estado) {}
};

// Clase CPU
class CPU {
public:
    std::vector<Proceso> procesos;

    void agregarProceso(const std::string& nombre, int id, int rafaga, int tiempoLlegada) {
        procesos.emplace_back(nombre, id, rafaga, tiempoLlegada);
    }
};

// Función para convertir ticks en una hora legible
std::string convertirTicksAHora(long long startTimeTicks) {
    long ticksPorSegundo = sysconf(_SC_CLK_TCK);
    long long tiempoInicioSegundos = startTimeTicks / ticksPorSegundo;

    auto ahora = std::chrono::system_clock::now();
    auto tiempoEpoch = std::chrono::duration_cast<std::chrono::seconds>(ahora.time_since_epoch()).count();

    std::ifstream uptimeFile("/proc/uptime");
    double tiempoEncendido;
    if (uptimeFile.is_open()) {
        uptimeFile >> tiempoEncendido;
    }

    long long tiempoInicioEpoch = tiempoEpoch - static_cast<long long>(tiempoEncendido) + tiempoInicioSegundos;
    std::time_t tiempoInicio = static_cast<std::time_t>(tiempoInicioEpoch);
    std::tm* tm = std::localtime(&tiempoInicio);

    std::ostringstream oss;
    oss << std::put_time(tm, "%H:%M:%S");
    return oss.str();
}

// Función para llenar la lista de procesos en la CPU
void capturarProcesos(CPU& cpu) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 10);

    int tiempoReferencia = -1;  

    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
        if (entry.is_directory()) {
            std::string dirName = entry.path().filename().string();

            if (std::all_of(dirName.begin(), dirName.end(), ::isdigit)) {
                try {
                    std::ifstream commFile(entry.path() / "comm");
                    std::string nombreProceso;
                    if (commFile.is_open() && std::getline(commFile, nombreProceso)) {
                        int pid = std::stoi(dirName);
                        int rafaga = distrib(gen);

                        std::ifstream statFile(entry.path() / "stat");
                        std::string statLine;
                        if (statFile.is_open() && std::getline(statFile, statLine)) {
                            std::istringstream iss(statLine);
                            // Usamos llaves para la inicialización del vector
                            std::vector<std::string> statTokens{
                                std::istream_iterator<std::string>(iss),
                                std::istream_iterator<std::string>()
                            };

                            // Verificamos que el vector tenga suficientes elementos
                            if (statTokens.size() > 21) {
                                try {
                                    long long startTimeTicks = std::stoll(statTokens[21]);
                                    int tiempoLlegada = 0;

                                    // Solo el primer proceso se toma como referencia con tiempo 0
                                    if (tiempoReferencia == -1) {
                                        tiempoReferencia = startTimeTicks / sysconf(_SC_CLK_TCK);  // Convertir ticks a segundos
                                    } else {
                                        int tiempoEnSegundos = startTimeTicks / sysconf(_SC_CLK_TCK);
                                        tiempoLlegada = tiempoEnSegundos - tiempoReferencia;
                                    }

                                    cpu.agregarProceso(nombreProceso, pid, rafaga, tiempoLlegada);
                                } catch (const std::invalid_argument& e) {
                                    // Si no se puede convertir, simplemente ignoramos ese proceso
                                    std::cerr << "Error al convertir el tiempo de inicio para el PID " << pid << std::endl;
                                }
                            } else {
                                std::cerr << "El proceso con PID " << pid << " no tiene suficientes datos en /proc/[PID]/stat." << std::endl;
                            }
                        }
                    }
                } catch (...) {
                    // Ignorar errores
                }
            }
        }
    }
}

int main() {
    CPU cpu;

    capturarProcesos(cpu);

    for (const auto& proceso : cpu.procesos) {
        std::cout << "Nombre: " << proceso.nombre
                  << ", PID: " << proceso.id
                  << ", Ráfaga: " << proceso.rafaga
                  << ", Hora de Llegada: " << proceso.tiempoLlegada
                  << ", Estado: " << proceso.estado
                  << std::endl;
    }

    return 0;
}
